/*
 * image_set.c
 *
 *  Created on: 18 Nov 2026
 *      Author: tyrrells
 */

#define ALLOCATE_IMAGE_OBJECT_TAGS (1)
#include "image_object.h"
#include "string_utils.h"
#include "time_util.h"
#include "image_set.h"


static bool GetImageObjectAsSchemaOrgObject (json_t *image_json_p, const ImageObject * const io_p, const struct tm * const date_s);

static int CompareNullableStrings (const char * const value_0_s, const char * const value_1_s);




ImageObject *AllocateImageObject (const char * const name_s, const char * const full_size_url_s, const char * const thumbnail_url_s)
{
	char *copied_name_s = EasyCopyToNewString (name_s);

	if (copied_name_s)
		{
			char *copied_full_size_url_s = NULL;

			if ((full_size_url_s == NULL) || ((copied_full_size_url_s = EasyCopyToNewString (full_size_url_s)) != NULL))
				{
					char *copied_thumbnail_url_s = NULL;

					if ((thumbnail_url_s == NULL) || ((copied_thumbnail_url_s = EasyCopyToNewString (thumbnail_url_s)) != NULL))
						{
							ImageObject *image_set_p = (ImageObject *) AllocMemory (sizeof (ImageObject));

							if (image_set_p)
								{
									image_set_p -> io_name_s = copied_name_s;
									image_set_p -> io_full_size_url_s = copied_full_size_url_s;
									image_set_p -> io_thumbnail_url_s = copied_thumbnail_url_s;

									return image_set_p;
								}
							else
								{
									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate ImageObject memory for \"%s\"", name_s);

								}

						}		/* if ((thumbnail_url_s == NULL) || ((copied_thumbnail_url_s = EasyCopyToNewString (thumbnail_url_s)) != NULL)) */
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "EasyCopyToNewString () failed for \"%s\"", thumbnail_url_s);
						}

				}		/* if ((full_size_url_s == NULL) || ((copied_full_size_url_s = EasyCopyToNewString (full_size_url_s)) != NULL)) */
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "EasyCopyToNewString () failed for \"%s\"", full_size_url_s);
				}

			FreeCopiedString (copied_name_s);
		}		/* if (copied_name_s) */
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "EasyCopyToNewString () failed for \"%s\"", name_s ? name_s : "NULL");
		}

	return NULL;
}


void FreeImageObject (ImageObject *image_set_p)
{
	if (image_set_p -> io_full_size_url_s)
		{
			FreeCopiedString (image_set_p -> io_full_size_url_s);
		}

	if (image_set_p -> io_thumbnail_url_s)
		{
			FreeCopiedString (image_set_p -> io_thumbnail_url_s);
		}

	FreeCopiedString (image_set_p -> io_name_s);

	FreeMemory (image_set_p);
}


ImageObjectNode *AllocateImageObjectNode (ImageObject *image_obj_p)
{
	ImageObjectNode *node_p = (ImageObjectNode *) AllocMemory (sizeof (ImageObjectNode));

	if (node_p)
		{
			InitListItem (& (node_p -> ion_node));
			node_p -> ion_image_obj_p = image_obj_p;

			return node_p;
		}
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "AllocateImageObjectNode () failed for \"%s\"", image_obj_p -> io_name_s);
		}

	return NULL;
}


void FreeImageObjectNode (ListItem *node_p)
{
	ImageObjectNode *io_node_p = (ImageObjectNode *) node_p;

	FreeImageObject (io_node_p -> ion_image_obj_p);

	FreeMemory (io_node_p);
}


const char *GetMainImageURL (const ImageObject * const image_obj_p)
{
	const char *url_s = image_obj_p -> io_full_size_url_s;

	if (!url_s)
		{
			url_s = image_obj_p -> io_thumbnail_url_s;
		}

	return url_s;
}


json_t *GetImageObjectAsJSON (const ImageObject * const io_p, const ImageSet * const parent_p, const ViewFormat vf)
{
	json_t *json_p = json_object ();

	if (json_p)
		{
			bool success_flag = true;

			switch (vf)
				{

					case VF_CLIENT_FULL:
					case VF_CLIENT_MINIMAL:
						{
							if (GetImageObjectAsSchemaOrgObject (json_p, io_p, parent_p -> is_date_p))
								{
									success_flag = true;
								}
							else
								{

								}
						}

						break;


					case VF_STORAGE:
					case VF_INDEXING:
					case VF_REFERENCE:
						{
							if (SetJSONString (json_p, IO_NAME_S, io_p -> io_name_s))
								{
									if (SetNonTrivialString (json_p, IO_FULL_IMAGE_URL_S, io_p -> io_full_size_url_s, true))
										{
											if (SetNonTrivialString (json_p, IO_THUMBNAIL_URL_S, io_p -> io_thumbnail_url_s, true))
												{
													success_flag = true;
												}		/* if (SetNonTrivialString (json_p, IO_THUMBNAIL_URL_S, io_p -> io_thumbnail_image_url_s, true)) */
											else
												{
													PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, json_p, "Failed to set \"%s\":\"%s\"", IO_THUMBNAIL_URL_S, io_p -> io_thumbnail_url_s);
												}

										}		/* if (SetNonTrivialString (json_p, IO_FULL_IMAGE_URL_S, io_p -> io_full_image_url_s, true)) */
									else
										{
											PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, json_p, "Failed to set \"%s\":\"%s\"", IO_FULL_IMAGE_URL_S, io_p -> io_full_size_url_s);
										}

								}		/* if (SetJSONString (json_p, IO_NAME_S, io_p -> io_name_s)) */
							else
								{
									PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, json_p, "Failed to set \"%s\":\"%s\"", IO_NAME_S, io_p -> io_name_s);
								}

						}
						break;

					case VF_NUM_FORMATS:
						success_flag = false;
						break;
				}


			if (success_flag)
				{
					return json_p;
				}



			json_decref (json_p);
		}
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate json object");
		}

	return NULL;
}


ImageObject *GetImageObjectFromJSON (const json_t * const json_p)
{
	ImageObject *image_obj_p = NULL;
	const char *name_s = GetJSONString (json_p, IO_NAME_S);

	if (name_s)
		{
			const char *full_image_s = GetJSONString (json_p, IO_FULL_IMAGE_URL_S);
			const char *thumbnail_image_s = GetJSONString (json_p, IO_THUMBNAIL_URL_S);

			if (full_image_s || thumbnail_image_s)
				{
					image_obj_p = AllocateImageObject (name_s, full_image_s, thumbnail_image_s);

					if (!image_obj_p)
						{
							PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, json_p, "AllocateImageObject () failed for \"%s\"", name_s);
						}
				}
			else
				{
					PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, json_p, "GetImageObjectFromJSON (): no full image or thumbnail specified for \"%s\"", name_s);
				}

		}		/* if (name_s) */
	else
		{
			PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, json_p, "GetImageObjectFromJSON (): no name specified");
		}

	return image_obj_p;
}



int CompareImageObjects (const ImageObject * const io_0_p, const ImageObject * const io_1_p)
{
	int res = CompareNullableStrings (io_0_p -> io_full_size_url_s, io_1_p -> io_full_size_url_s);

	if (res == 0)
		{
			res = CompareNullableStrings (io_0_p -> io_thumbnail_url_s, io_1_p -> io_thumbnail_url_s);
		}

	return res;
}


/*
 * STATIC DEFINITIONS
 */


static bool GetImageObjectAsSchemaOrgObject (json_t *image_json_p, const ImageObject * const io_p, const struct tm * const date_p)
{
/*
	"@context": "https://schema.org",
  "@type": "ImageSet",
  "author": "Jane Doe",
  "contentLocation": "Puerto Vallarta, Mexico",
  "contentUrl": "mexico-beach.jpg",
  "datePublished": "2008-01-25",
  "description": "I took this picture while on vacation last year.",
  "name": "Beach in Mexico"
*/

	bool success_flag = false;


	if (SetJSONString (image_json_p, "@type", "ImageObject"))
		{
			if (SetJSONString (image_json_p, "name", io_p -> io_name_s))
				{
					if ((io_p -> io_full_size_url_s == NULL) || (SetJSONString (image_json_p, "contentUrl", io_p -> io_full_size_url_s)))
						{
							if ((io_p -> io_thumbnail_url_s == NULL) || (SetJSONString (image_json_p, "thumbnailUrl", io_p -> io_thumbnail_url_s)))
								{
									if (date_p)
										{
											char *time_s = GetTimeAsString (date_p, false, NULL);

											if (time_s)
												{
													if (SetJSONString (image_json_p, "datePublished", time_s))
														{
															success_flag = true;
														}

													FreeCopiedString (time_s);
												}

										}
									else
										{
											success_flag = true;
										}


								}		/* if ((io_p -> io_full_size_url_s == NULL) || (SetJSONString (image_json_p, "contentUrl", io_p -> io_full_size_url_s))) */
							else
								{

								}

						}		/* if ((io_p -> io_full_size_url_s == NULL) || (SetJSONString (image_json_p, "contentUrl", io_p -> io_full_size_url_s))) */
					else
						{

						}
				}		/* if (SetJSONString (image_json_p, "name", io_p -> io_name_s)) */
			else
				{

				}



		}		/* if (SetJSONString (image_json_p, "@type", "ImageObject")) */
	else
		{

		}



  return success_flag;
}



static int CompareNullableStrings (const char * const value_0_s, const char * const value_1_s)
{
	int res = 0;

	if (value_0_s)
		{
			if (value_1_s)
				{
					res = strcmp (value_0_s, value_1_s);
				}
			else
				{
					res = -1;
				}
		}
	else
		{
			if (value_1_s)
				{
					res = 1;
				}
		}

	return res;
}
