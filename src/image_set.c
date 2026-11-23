/*
 * image_object.c
 *
 *  Created on: 17 Oct 2026
 *      Author: tyrrells
 */


#define ALLOCATE_IMAGE_SET_TAGS (1)
#include "image_set.h"
#include "memory_allocations.h"
#include "streams.h"
#include "time_util.h"


/*
 * STATIC DECLARATIONS
 */


static bool AddImageObjectToJSON (json_t *images_json_p, const ImageObject * const io_p, const ImageSet * const is_p, const ViewFormat vf);

static ImageObjectNode *GetNamedImageObjectNode (const ImageSet *image_set_p, const char * const name_s);


static const char * const S_WAVELENGTHS_SS [WA_NUM_TYPES] =
{
	"RGB",
	"Hyperspectral",
	"Thermal"
};

/*
 * API DEFINITIONS
 */


ImageSet *AllocateImageSet (struct tm *src_date_p, const uint32 index, const Wavelength wavelength)
{
	LinkedList *image_objects_p = AllocateLinkedList (FreeImageSetNode);

	if (image_objects_p)
		{
			struct tm *dest_date_p = NULL;
			bool got_date_flag = false;

			if (src_date_p)
				{
					dest_date_p = DuplicateTime (src_date_p);

					if (dest_date_p)
						{
							got_date_flag = true;
						}
					else
						{

						}
				}
			else
				{
					got_date_flag = true;
				}

			if (got_date_flag)
				{
					ImageSet *image_set_p = AllocMemory (sizeof (ImageSet));

					if (image_set_p)
						{
							image_set_p -> is_date_p = dest_date_p;
							image_set_p -> is_index = index;
							image_set_p -> is_wavelength = wavelength;

							image_set_p -> is_images_p = image_objects_p;

							return image_set_p;
						}

					if (dest_date_p)
						{
							FreeTime (dest_date_p);
						}
				}

			FreeLinkedList (image_objects_p);
		}

	return NULL;
}


void FreeImageSet (ImageSet *image_set_p)
{
	if (image_set_p -> is_date_p)
		{
			FreeTime (image_set_p -> is_date_p);
		}

	FreeLinkedList (image_set_p -> is_images_p);

	FreeMemory (image_set_p);
}



OperationStatus AddImageObjectToImageSet (ImageSet *image_set_p, ImageObject *image_obj_p)
{
	OperationStatus status = OS_IDLE;
	ImageObjectNode *node_p = AllocateImageObjectNode (image_obj_p);

	if (node_p)
		{
			LinkedListAddTail (image_set_p -> is_images_p, & (node_p -> ion_node));
			status = OS_SUCCEEDED;
		}
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "AllocateImageObjectNode () failed for \"%s\" (\"%s\")",
									 image_obj_p -> io_name_s, GetMainImageURL (image_obj_p));
			status = OS_FAILED;
		}

	return status;
}


json_t *GetImageSetAsJSON (const ImageSet * const image_set_p, const ViewFormat vf)
{
	json_t *json_p = json_object ();

	if (json_p)
		{
			bool success_flag = false;

			char *time_s = GetTimeAsString (image_set_p -> is_date_p, false, NULL);

			if (time_s)
				{
					if (SetNonTrivialString (json_p, IS_DATE_S, time_s, true))
						{
							if (SetNonTrivialUnsignedInt (json_p, IS_INDEX_S, & (image_set_p -> is_index), false))
								{
									const char *wavelength_s = GetWavelengthAsString (image_set_p -> is_wavelength);

									if (wavelength_s)
										{
											if (SetJSONString (json_p, IS_WAVELENGTH_S, wavelength_s))
												{
													if (image_set_p -> is_images_p -> ll_size > 0)
														{
															json_t *image_objects_json_p = json_array ();

															if (image_objects_json_p)
																{
																	if (json_object_set_new (json_p, IS_IMAGES_S, image_objects_json_p) == 0)
																		{
																			ImageObjectNode *io_node_p = (ImageObjectNode *) (image_set_p -> is_images_p -> ll_head_p);
																			bool added_flag = true;

																			while (io_node_p && added_flag)
																				{
																					if (AddImageObjectToJSON (json_p, io_node_p -> ion_image_obj_p, image_set_p, vf))
																						{
																							io_node_p = (ImageObjectNode *) (io_node_p -> ion_node.ln_next_p);
																						}
																					else
																						{
																							added_flag = false;
																						}
																				}		/* while (io_node_p) */

																			if (added_flag)
																				{
																					success_flag = true;
																				}
																		}
																	else
																		{

																			json_decref (image_objects_json_p);
																		}
																}



														}
												}
											else
												{
													PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, json_p, "Failed to set \"%s\":\"%s\"", IS_WAVELENGTH_S, wavelength_s);
												}

										}
									else
										{

										}

								}
							else
								{
									PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, json_p, "Failed to set \"%s\": " UINT32_FMT, IS_INDEX_S, image_set_p -> is_index);
								}

						}		/* if (SetNonTrivialString (json_p, IO_DATE_S, time_s, true)) */
					else
						{
							PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, json_p, "Failed to set \"%s\":\"%s\"", IS_DATE_S, time_s);
						}

					FreeCopiedString (time_s);
				}

			if (success_flag)
				{
					return json_p;
				}
			else
				{
					json_decref (json_p);
				}

		}
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate json for ImageSet");
		}

	return NULL;
}


ImageSet *GetImageSetFromJSON (const json_t * const json_p)
{
	ImageSet *image_set_p = NULL;

	struct tm *image_date_p = NULL;
	const char *date_s = GetJSONString (json_p, IS_DATE_S);
	const char *wavelength_s = GetJSONString (json_p, IS_WAVELENGTH_S);
	uint32 image_index = 1;
	Wavelength wavelength = WA_NUM_TYPES;

	GetJSONUnsignedInteger (json_p, IS_INDEX_S, &image_index);

	if (date_s)
		{
			image_date_p = GetTimeFromString (date_s);
		}
	else
		{

		}

	if (wavelength_s)
		{
			if (GetWavelengthFromString (wavelength_s, &wavelength))
				{

				}
		}

	image_set_p = AllocateImageSet (image_date_p, image_index, wavelength);

	if (!image_set_p)
		{
			PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, json_p, "AllocateImageSet () failed");
		}

	if (image_date_p)
		{
			FreeTime (image_date_p);
		}


	return image_set_p;
}


ImageSetNode *AllocateImageSetNode (ImageSet *image_p)
{
	ImageSetNode *node_p = (ImageSetNode *) AllocMemory (sizeof (ImageSetNode));

	if (node_p)
		{
			InitListItem (& (node_p -> isn_base_node));
			node_p -> isn_image_p = image_p;
		}
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate memory for ImageSetNode");
		}

	return node_p;
}


void FreeImageSetNode (ListItem *node_p)
{
	ImageSetNode *image_node_p = (ImageSetNode *) node_p;

	FreeImageSet (image_node_p -> isn_image_p);
	FreeMemory (image_node_p);
}



int CompareImageSets (const ImageSet * const is0_p, const ImageSet * const is1_p)
{
	const bool dates_only_flag = true;
	int res = CompareDates (is0_p -> is_date_p, is1_p -> is_date_p, dates_only_flag);

	if (res == 0)
		{
			res = (is0_p -> is_index) - (is1_p -> is_index);

			if (res == 0)
				{
					res = (is0_p -> is_wavelength) - (is1_p -> is_wavelength);
				}
		}

	return res;
}



const char *GetWavelengthAsString (const Wavelength w)
{
	const char *wavelength_s = NULL;

	if ((w >= 0) && (w < WA_NUM_TYPES))
		{
			wavelength_s = * (S_WAVELENGTHS_SS + w);
		}

	return wavelength_s;
}


bool GetWavelengthFromString (const char * const wavelength_s, Wavelength *w_p)
{
	bool success_flag = false;
	const char **value_ss = (const char **) S_WAVELENGTHS_SS;
	Wavelength w = 0;

	while ((!success_flag) && (w < WA_NUM_TYPES))
		{
			if (strcmp (wavelength_s, *value_ss) == 0)
				{
					*w_p = w;
					success_flag = true;
				}
			else
				{
					++ w;
					++ value_ss;
				}
		}

	return success_flag;
}


ImageObject *GetNamedImageObjectFromImageSet (const ImageSet *image_set_p, const char * const name_s)
{
	ImageObject *io_p = NULL;
	ImageObjectNode *node_p = GetNamedImageObjectNode (image_set_p, name_s);

	if (node_p)
		{
			io_p = node_p -> ion_image_obj_p;
		}

	return io_p;
}



static ImageObjectNode *GetNamedImageObjectNode (const ImageSet *image_set_p, const char * const name_s)
{
	ImageObjectNode *node_p = (ImageObjectNode *) (image_set_p -> is_images_p -> ll_head_p);

	while (node_p)
		{
			if (strcmp (node_p -> ion_image_obj_p -> io_name_s, name_s) == 0)
				{
					return node_p;
				}
			else
				{
					node_p = (ImageObjectNode *) (node_p -> ion_node.ln_next_p);
				}
		}

	return NULL;
}




static bool AddImageObjectToJSON (json_t *images_json_p, const ImageObject * const io_p, const ImageSet * const is_p, const ViewFormat vf)
{
	bool success_flag = false;
	json_t *io_json_p = GetImageObjectAsJSON (io_p, is_p, vf);

	if (io_json_p)
		{
			if (json_array_append_new (images_json_p, io_json_p) == 0)
				{
					success_flag = true;
				}
			else
				{
					PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, io_json_p, "Failed to add to images array");
				}
		}
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to get ImageObject \"%s\" as JSON", io_p -> io_name_s);
		}


	return success_flag;
}
