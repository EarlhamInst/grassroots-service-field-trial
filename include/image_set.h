/*
 * image_object.h
 *
 *  Created on: 17 Oct 2026
 *      Author: tyrrells
 */

#ifndef SERVICES_FIELD_TRIALS_INCLUDE_IMAGE_SET_H_
#define SERVICES_FIELD_TRIALS_INCLUDE_IMAGE_SET_H_

#include "linked_list.h"


#include "dfw_field_trial_service_data.h"
#include "dfw_field_trial_service_library.h"

#include "image_object.h"



typedef enum Wavelength
{
	WA_RGB,

	WA_HYPERSPECTRAL,

	WA_THERMAL,

	WA_NUM_TYPES
} Wavelength;



/**
 * A datatype to hold the various image urls and metadata
 * that a plot, study, etc. may have.
 */
typedef struct ImageSet
{


	/** The date of the image. */
	struct tm *is_date_p;

	/**
	 * The index, starting from 1, for the image within
	 * possible multiple runs.
	 */
	uint32 is_index;

	/**
	 * The wavelength of the image e.g. RFB, HyperSpectral, Thermal, etc.
	 */
	Wavelength is_wavelength;

	/**
	 * A LinkedList of ImageObjectNodes.
	 */
	LinkedList *is_images_p;

} ImageSet;


typedef struct ImageSetNode
{
	ListItem isn_base_node;

	ImageSet *isn_image_p;

} ImageSetNode;


#ifndef DOXYGEN_SHOULD_SKIP_THIS

#ifdef ALLOCATE_IMAGE_SET_TAGS
	#define IMAGE_SET_PREFIX DFW_FIELD_TRIAL_SERVICE_LOCAL
	#define IMAGE_SET_VAL(x)	= x
	#define IMAGE_SET_CONCAT_VAL(x,y)	= x y
#else
	#define IMAGE_SET_PREFIX extern
	#define IMAGE_SET_VAL(x)
	#define IMAGE_SET_CONCAT_VAL(x,y)
#endif

#endif 		/* #ifndef DOXYGEN_SHOULD_SKIP_THIS */


IMAGE_SET_PREFIX const char *IS_IMAGES_S IMAGE_SET_VAL ("images");


IMAGE_SET_PREFIX const char *IS_DATE_S IMAGE_SET_CONCAT_VAL (CONTEXT_PREFIX_SCHEMA_ORG_S, "date");

IMAGE_SET_PREFIX const char *IS_INDEX_S IMAGE_SET_VAL ("index");

IMAGE_SET_PREFIX const char *IS_WAVELENGTH_S IMAGE_SET_VAL ("wavelength");


#ifdef __cplusplus
extern "C"
{
#endif


DFW_FIELD_TRIAL_SERVICE_LOCAL ImageSet *AllocateImageSet (struct tm *date_p, const uint32 index, const Wavelength wavelength);


DFW_FIELD_TRIAL_SERVICE_LOCAL void FreeImageSet (ImageSet *image_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL json_t *GetImageSetAsJSON (const ImageSet * const image_p, const ViewFormat vf);


DFW_FIELD_TRIAL_SERVICE_LOCAL ImageSet *GetImageSetFromJSON (const json_t * const json_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL ImageSetNode *AllocateImageSetNode (ImageSet *image_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL void FreeImageSetNode (ListItem *node_p);



DFW_FIELD_TRIAL_SERVICE_LOCAL int CompareImageSets (const ImageSet * const image_set_0_p, const ImageSet * const image_set_1_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL const char *GetWavelengthAsString (const Wavelength w);

DFW_FIELD_TRIAL_SERVICE_LOCAL bool GetWavelengthFromString (const char * const wavelength_s, Wavelength *w_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL OperationStatus AddImageObjectToImageSet (ImageSet *image_set_p, ImageObject *image_obj_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL ImageObject *GetNamedImageObjectFromImageSet (const ImageSet *image_set_p, const char * const name_s);



#ifdef __cplusplus
}
#endif


#endif /* SERVICES_FIELD_TRIALS_INCLUDE_IMAGE_SET_H_ */
