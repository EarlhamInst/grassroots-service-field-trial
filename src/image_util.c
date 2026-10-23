/*
** Copyright 2014-2020 The Earlham Institute
** 
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
** 
**     http://www.apache.org/licenses/LICENSE-2.0
** 
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/
/*
 * image_util.c
 *
 *  Created on: 29 Mar 2020
 *      Author: billy
 */

#include <ctype.h>

#include "libexif/exif-data.h"

#include "image_util.h"
#include "time_util.h"
#include "memory_allocations.h"



static bool GetImageDimension (ExifData *exif_p, const ExifTag tag, uint32 *value_p);
static bool GetGPSValue (ExifContent *gps_content_p, double *gps_value_p, const ExifTag direction_tag, const ExifTag reference_tag, const char negative_reference_value);
static struct tm *GetDatestamp (ExifData *exif_p);
static Coordinate *GetGPSCoordinate (ExifData *exif_p);


ImageMetadata *GetImageMetadataForImageFile (const char *path_s)
{
  ExifData *exif_p = exif_data_new_from_file (path_s);

  if (exif_p)
    {
  		Coordinate *coord_p = GetGPSCoordinate (exif_p);

  		if (coord_p)
  			{
					struct tm *datestamp_p = GetDatestamp (exif_p);

					if (datestamp_p)
						{
							uint32 width;

							if (GetImageDimension (exif_p, EXIF_TAG_PIXEL_X_DIMENSION, &width))
								{
									uint32 height;

									if (GetImageDimension (exif_p, EXIF_TAG_PIXEL_Y_DIMENSION, &height))
										{
											ImageMetadata *metadata_p = AllocateImageMetadata (coord_p, datestamp_p, width, height);

											if (metadata_p)
												{
													return metadata_p;
												}

										}

								}

							FreeTime (datestamp_p);
						}

					FreeCoordinate (coord_p);
				}

      exif_data_unref (exif_p);
    }
  else
  	{

  	}

  return NULL;
}


ImageMetadata *AllocateImageMetadata (Coordinate *coord_p, struct tm *time_p, uint32 width, uint32 height)
{
	ImageMetadata *metadata_p = (ImageMetadata *) AllocMemory (sizeof (ImageMetadata));

	if (metadata_p)
		{
			metadata_p -> im_coord_p = coord_p;
			metadata_p -> im_date_p = time_p;
			metadata_p -> im_width = width;
			metadata_p -> im_height = height;

			return metadata_p;
		}

	return NULL;
}


void FreeImageMetadata (ImageMetadata *metadata_p)
{
	FreeCoordinate (metadata_p -> im_coord_p);
	FreeTime (metadata_p -> im_date_p);
	FreeMemory (metadata_p);
}


static bool GetImageDimension (ExifData *exif_p, const ExifTag tag, uint32 *value_p)
{
	bool success_flag = false;
	ExifEntry *entry_p = exif_data_get_entry (exif_p, tag);

	if (entry_p)
		{
			if (entry_p -> format == EXIF_FORMAT_LONG)
				{
					const ExifByteOrder byte_order = exif_data_get_byte_order (exif_p);
					ExifLong l = exif_get_long (entry_p -> data, byte_order);

					*value_p = l;
					success_flag = true;
				}
		}		/* if (entry_p) */

	return success_flag;
}


static struct tm *GetDatestamp (ExifData *exif_p)
{
	ExifEntry *entry_p = exif_data_get_entry (exif_p, EXIF_TAG_DATE_TIME_ORIGINAL);

	if (entry_p)
		{
			if (entry_p -> format == EXIF_FORMAT_ASCII)
				{
					struct tm *time_p = GetTimeFromString ((const char *) (entry_p -> data));

					return time_p;
				}
		}

	return NULL;
}


static Coordinate *GetGPSCoordinate (ExifData *exif_p)
{
	ExifContent *gps_content_p = exif_p -> ifd [EXIF_IFD_GPS];

	if (gps_content_p)
		{
			double latitude;

			if (GetGPSValue (gps_content_p, &latitude, EXIF_TAG_GPS_LATITUDE, EXIF_TAG_GPS_LATITUDE_REF, 'S'))
				{
					double longitude;

					if (GetGPSValue (gps_content_p, &longitude, EXIF_TAG_GPS_LONGITUDE, EXIF_TAG_GPS_LONGITUDE_REF, 'W'))
						{
							Coordinate *coord_p = AllocateCoordinate (latitude, longitude);

							if (coord_p)
								{
									return coord_p;
								}
						}
				}
		}

	return NULL;
}


static bool GetGPSValue (ExifContent *gps_content_p, double *gps_value_p, const ExifTag direction_tag, const ExifTag reference_tag, const char negative_reference_value)
{
	ExifEntry *value_p = exif_content_get_entry (gps_content_p, direction_tag);

	if (value_p)
		{
			/*
			 * Does it have 3 components for the hours, minutes and seconds?
			*/
			if (value_p -> components == 3)
				{
					if (value_p -> format == EXIF_FORMAT_RATIONAL)
						{
							const ExifByteOrder byte_order = exif_data_get_byte_order (gps_content_p -> parent);
							unsigned char *data_p = value_p -> data;
							unsigned int size = exif_format_get_size (value_p -> format);
							double hours;
							double minutes;
							double seconds;
							double value;

							/* hours */
							ExifRational r = exif_get_rational (data_p, byte_order);
							hours = ((double) r.numerator) / ((double) r.denominator);

							/* minutes */
							data_p += size;
							r = exif_get_rational (data_p, byte_order);
							minutes = ((double) r.numerator) / ((double) r.denominator);

							/* seconds */
							data_p += size;
							r = exif_get_rational (data_p, byte_order);
							seconds = ((double) r.numerator) / ((double) r.denominator);

							value = hours + (minutes / 60.0) + (seconds / 3600.0);

							/* is it above or below the equator? N for North or S for South */
							value_p = exif_content_get_entry (gps_content_p, reference_tag);
							if (value_p)
								{
									char direction = '\0';
									exif_entry_get_value (value_p, &direction, 1);

									if ((direction == toupper (negative_reference_value)) || (direction == tolower (negative_reference_value)))
										{
											value = -value;
										}
								}

							*gps_value_p = value;

							return true;
						}
				}
		}

	return false;
}
