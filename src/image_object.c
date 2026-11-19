/*
 * image_set.c
 *
 *  Created on: 18 Nov 2026
 *      Author: tyrrells
 */

#define ALLOCATE_IMAGE_OBJECT_TAGS (1)
#include "image_object.h"
#include "string_utils.h"


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


json_t *GetImageObjectAsJSON (const ImageObject * const image_p, const ViewFormat vf)
{
	json_t *json_p = json_object ();

	if (json_p)
		{
			if (SetJSONString (json_p, IO_NAME_S, image_p -> io_name_s))
				{
					if ((image_p -> io_full_size_url_s == NULL) && (SetJSONString (json_p, IO_FULL_IMAGE_URL_S, image_p -> io_full_size_url_s)))
						{
							if ((image_p -> io_thumbnail_url_s == NULL) && (SetJSONString (json_p, IO_THUMBNAIL_URL_S, image_p -> io_thumbnail_url_s)))
								{
									return json_p;
								}
							else
								{
									PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, json_p, "GetImageObjectAsJSON (): Failed to set \"%s\": \"%s\"", IO_FULL_IMAGE_URL_S, image_p -> io_thumbnail_url_s);
								}
						}
					else
						{
							PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, json_p, "GetImageObjectAsJSON (): Failed to set \"%s\": \"%s\"", IO_FULL_IMAGE_URL_S, image_p -> io_full_size_url_s);
						}

				}
			else
				{
					PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, json_p, "GetImageObjectAsJSON (): Failed to set \"%s\": \"%s\"", IO_NAME_S, image_p -> io_name_s);
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
							PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, json_p, "AllocateImageObject () failed");
						}
				}
			else
				{
					PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, json_p, "GetImageObjectFromJSON (): no full image or thumbnail specified");
				}

		}		/* if (name_s) */
	else
		{
			PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, json_p, "GetImageObjectFromJSON (): no name specified");
		}

	return image_obj_p;
}
