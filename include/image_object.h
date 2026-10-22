/*
 * image_object.h
 *
 *  Created on: 17 Oct 2026
 *      Author: tyrrells
 */

#ifndef SERVICES_FIELD_TRIALS_INCLUDE_IMAGE_OBJECT_H_
#define SERVICES_FIELD_TRIALS_INCLUDE_IMAGE_OBJECT_H_

#include "linked_list.h"


#include "dfw_field_trial_service_data.h"
#include "dfw_field_trial_service_library.h"



/**
 * A datatype to hold the various image urls that a plot, study, etc.
 * may have.
 */
typedef struct ImageObject
{
	/** The web address for the full size image */
	char *io_full_image_url_s;

	/** The web address for the thumbnail image */
	char *io_thumbnail_image_url_s;
} ImageObject;


typedef struct ImageObjectNode
{
	ListItem ion_base_node;

	ImageObject *ion_image_p;

} ImageObjectNode;


#ifndef DOXYGEN_SHOULD_SKIP_THIS

#ifdef ALLOCATE_IMAGE_OBJECT_TAGS
	#define IMAGE_OBJECT_PREFIX DFW_FIELD_TRIAL_SERVICE_LOCAL
	#define IMAGE_OBJECT_VAL(x)	= x
	#define IMAGE_OBJECT_CONCAT_VAL(x,y)	= x y
#else
	#define IMAGE_OBJECT_PREFIX extern
	#define IMAGE_OBJECT_VAL(x)
	#define IMAGE_OBJECT_CONCAT_VAL(x,y)
#endif

#endif 		/* #ifndef DOXYGEN_SHOULD_SKIP_THIS */


IMAGE_OBJECT_PREFIX const char *IO_FULL_IMAGE_URL_S IMAGE_OBJECT_CONCAT_VAL (CONTEXT_PREFIX_SCHEMA_ORG_S, "url");

IMAGE_OBJECT_PREFIX const char *IO_THUMBNAIL_URL_S IMAGE_OBJECT_CONCAT_VAL (CONTEXT_PREFIX_SCHEMA_ORG_S, "url");



#ifdef __cplusplus
extern "C"
{
#endif


DFW_FIELD_TRIAL_SERVICE_LOCAL ImageObject *AllocateImageObject (const char * const image_s, const char * const thumbnail_s);


DFW_FIELD_TRIAL_SERVICE_LOCAL void FreeImageObject (ImageObject *image_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL json_t *GetImageObjectAsJSON (const ImageObject * const image_p, const ViewFormat vf);


DFW_FIELD_TRIAL_SERVICE_LOCAL ImageObject *GetImageObjectFromJSON (const json_t * const json_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL ImageObjectNode *AllocateImageObjectNode (ImageObject *image_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL void FreeImageObjectNode (ListItem *node_p);



DFW_FIELD_TRIAL_SERVICE_LOCAL int CompareImageObjects (const ImageObject * const image_0_p, const ImageObject * const image_1_p);




#ifdef __cplusplus
}
#endif


#endif /* SERVICES_FIELD_TRIALS_INCLUDE_IMAGE_OBJECT_H_ */
