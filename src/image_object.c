/*
 * image_object.c
 *
 *  Created on: 17 Oct 2026
 *      Author: tyrrells
 */


#define ALLOCATE_IMAGE_OBJECT_TAGS (1)
#include "image_object.h"
#include "memory_allocations.h"
#include "streams.h"


/*
 * STATIC DECLARATIONS
 */

static bool GetImageObjectAsSchemaOrgObject (json_t *image_json_p, const ImageObject * const image_p);


/*
 * API DEFINITIONS
 */

ImageObject *AllocateImageObject (const char * const image_s, const char * const thumbnail_s)
{
	if (image_s || thumbnail_s)
		{
			char *copied_image_s = NULL;

			if ((image_s == NULL) || ((copied_image_s = EasyCopyToNewString (image_s)) != NULL))
				{
					char *copied_thumbnail_s = NULL;

					if ((thumbnail_s == NULL) || ((copied_thumbnail_s = EasyCopyToNewString (thumbnail_s)) != NULL))
						{
							ImageObject *image_p = AllocMemory (sizeof (ImageObject));

							if (image_p)
								{
									image_p -> io_full_image_url_s = copied_image_s;
									image_p -> io_thumbnail_image_url_s = copied_thumbnail_s;

									return image_p;
								}

							if (copied_thumbnail_s)
								{
									FreeCopiedString (copied_thumbnail_s);
								}
						}
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to copy thumbnail \"%s\"", thumbnail_s);
						}

					if (copied_image_s)
						{
							FreeCopiedString (copied_image_s );
						}

				}
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to copy image \"%s\"", image_s);
				}

		}
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "image and thumbnail are both NULL");
		}


	return NULL;
}


void FreeImageObject (ImageObject *image_p)
{
	if (image_p -> io_thumbnail_image_url_s)
		{
			FreeCopiedString (image_p -> io_thumbnail_image_url_s);
		}

	if (image_p -> io_full_image_url_s)
		{
			FreeCopiedString (image_p -> io_full_image_url_s);
		}

	FreeMemory (image_p);
}


json_t *GetImageObjectAsJSON (const ImageObject * const image_p, const ViewFormat vf)
{
	json_t *json_p = json_object ();

	if (json_p)
		{
			bool success_flag = false;

			switch (vf)
				{
					case VF_CLIENT_FULL:
						{
							if (GetImageObjectAsSchemaOrgObject (json_p, image_p))
								{
									success_flag = true;
								}
							else
								{

								}
						}

						break;

					case VF_CLIENT_MINIMAL:

						break;


					case VF_STORAGE:
						{
							if ((image_p -> io_full_image_url_s == NULL) || (SetJSONString (json_p, IO_FULL_IMAGE_URL_S, image_p -> io_full_image_url_s)))
								{
									if ((image_p -> io_thumbnail_image_url_s == NULL) || (SetJSONString (json_p, IO_THUMBNAIL_URL_S, image_p -> io_thumbnail_image_url_s)))
										{
											success_flag = true;
										}
									else
										{
											PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, json_p, "Failed to set \"%s\":\"%s\"", IO_THUMBNAIL_URL_S, image_p -> io_thumbnail_image_url_s);
										}

								}
							else
								{
									PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, json_p, "Failed to set \"%s\":\"%s\"", IO_FULL_IMAGE_URL_S, image_p -> io_full_image_url_s);
								}

						}
						break;

					case VF_INDEXING:
					case VF_REFERENCE:
						break;

					case VF_NUM_FORMATS:
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
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate json for ImageObject \"%s\"",
									 image_p -> io_full_image_url_s ? image_p -> io_full_image_url_s :image_p -> io_thumbnail_image_url_s);

		}

	return NULL;
}


ImageObject *GetImageObjectFromJSON (const json_t * const json_p)
{
	const char *full_image_s = GetJSONString (json_p, IO_FULL_IMAGE_URL_S);
	const char *thumbnail_image_s = GetJSONString (json_p, IO_THUMBNAIL_URL_S);

	if (full_image_s || thumbnail_image_s)
		{
			ImageObject *image_p = AllocateImageObject (full_image_s, thumbnail_image_s);

			if (image_p)
				{
					return image_p;
				}
			else
				{
					PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, json_p, "AllocateImageObject () failed for \"%s\" and \"%s\"", full_image_s, thumbnail_image_s);
				}
		}
	else
		{
			PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, json_p, "No image details specified");
		}

	return NULL;
}


ImageObjectNode *AllocateImageObjectNode (ImageObject *image_p)
{
	ImageObjectNode *node_p = (ImageObjectNode *) AllocMemory (sizeof (ImageObjectNode));

	if (node_p)
		{
			InitListItem (& (node_p ->ion_base_node));
			node_p -> ion_image_p = image_p;
		}
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate memory for ImageObjectNode for \"%s\"",
									 image_p -> io_full_image_url_s ? image_p -> io_full_image_url_s :image_p -> io_thumbnail_image_url_s);
		}

	return node_p;
}


void FreeImageObjectNode (ListItem *node_p)
{
	ImageObjectNode *image_node_p = (ImageObjectNode *) node_p;

	FreeImageObject (image_node_p -> ion_image_p);
	FreeMemory (image_node_p);
}


/*
 * STATIC DEFINITIONS
 */


static bool GetImageObjectAsSchemaOrgObject (json_t *image_json_p, const ImageObject * const image_p)
{
/*
	"@context": "https://schema.org",
  "@type": "ImageObject",
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
			if (SetJSONString (image_json_p, "contentUrl", image_p -> io_full_image_url_s))
				{
					if ((image_p ->io_thumbnail_image_url_s == NULL) || (SetJSONString (image_json_p, "thumbnail", image_p ->io_thumbnail_image_url_s)))
						{
							success_flag = true;
						}		/* if (SetJSONString (image_json_p, "thumbnail, thumbnail_url_s)) */
					else
						{
							PrintJSONToErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, image_json_p, "Failed to set \"thumbnail\": \"%s\"", image_p ->io_thumbnail_image_url_s);
						}

				}		/* if (SetJSONString (image_json_p, "contentUrl", image_url_s)) */
			else
				{
					PrintJSONToErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, image_json_p, "Failed to set \"contentUrl\": \"%s\"", image_p -> io_full_image_url_s ? image_p -> io_full_image_url_s : image_p -> io_thumbnail_image_url_s);
				}

		}		/* if (SetJSONString (image_json_p, "@type", "ImageObject")) */
	else
		{
			PrintJSONToErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, image_json_p, "Failed to set \"@type\": \"ImageObject\" for \"%s\"", image_p -> io_full_image_url_s ? image_p -> io_full_image_url_s : image_p -> io_thumbnail_image_url_s);
		}

  return success_flag;
}

