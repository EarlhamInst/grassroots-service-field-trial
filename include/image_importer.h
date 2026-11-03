/*
 * image_importer.h
 *
 *  Created on: 20 Oct 2026
 *      Author: tyrrells
 */

#ifndef SERVICES_FIELD_TRIALS_INCLUDE_IMAGE_IMPORTER_H_
#define SERVICES_FIELD_TRIALS_INCLUDE_IMAGE_IMPORTER_H_

#include "dfw_field_trial_service_data.h"
#include "dfw_field_trial_service_library.h"


#ifdef __cplusplus
extern "C"
{
#endif


DFW_FIELD_TRIAL_SERVICE_LOCAL	OperationStatus RunImportImages (const char * const import_path_s, const FieldTrialServiceData * const data_p);


#ifdef __cplusplus
}
#endif


#endif /* SERVICES_FIELD_TRIALS_INCLUDE_IMAGE_IMPORTER_H_ */
