/*
 * brapi_importer.h
 *
 *  Created on: 2 Nov 2026
 *      Author: tyrrells
 */

#ifndef SERVICES_FIELD_TRIALS_INCLUDE_BRAPI_IMPORTER_H_
#define SERVICES_FIELD_TRIALS_INCLUDE_BRAPI_IMPORTER_H_

#include "dfw_field_trial_service_data.h"
#include "dfw_field_trial_service_library.h"

#include "service.h"


#ifdef __cplusplus
extern "C"
{
#endif


/**
 * Import Programmes, Field Trials, Studies, Plots and Observations from a
 * BrAPI v2 server such as BreedBase.
 *
 * The BrAPI types are mapped onto the Grassroots ones as follows:
 *
 * - program -> Programme
 * - trial -> Field Trial
 * - study -> Study (and its location -> Location)
 * - plot-level observationUnit -> Plot with a single StandardRow
 * - observation -> Observation on that StandardRow
 *
 * Existing Programmes and Field Trials with the same name are reused, while
 * any Study whose name already exists is skipped completely so the import
 * can safely be run more than once. Observations are only imported for
 * variables that already exist as Measured Variables in Grassroots,
 * matched against either the full BrAPI variable name or, for BreedBase's
 * "trait name|CO_xxx:yyyyyyy" style of names, either half of it.
 *
 * @param brapi_url_s The root of the BrAPI v2 server, e.g.
 * "https://wheat.triticeaetoolbox.org/brapi/v2".
 * @param auth_token_s An optional bearer token for servers that need
 * authentication. This can be <code>NULL</code>.
 * @param program_db_id_s If this is set, only the BrAPI program with this
 * programDbId will be imported. If it is <code>NULL</code> then all programs
 * will be imported.
 * @param gene_bank_name_s The name of the Gene Bank to use for any Materials
 * that need creating for the germplasm. If this is <code>NULL</code>, the
 * GRU will be used.
 * @param user_p The User who will be set as the owner of the imported data.
 * This can be <code>NULL</code>.
 * @param job_p The ServiceJob to store the results and any errors in.
 * @param data_p The configuration data for the Field Trial service.
 * @return The status of the import.
 */
DFW_FIELD_TRIAL_SERVICE_LOCAL OperationStatus ImportFromBrAPI (const char * const brapi_url_s, const char * const auth_token_s, const char * const program_db_id_s,
																															 const char * const gene_bank_name_s, User *user_p, ServiceJob *job_p, FieldTrialServiceData *data_p);


#ifdef __cplusplus
}
#endif


#endif /* SERVICES_FIELD_TRIALS_INCLUDE_BRAPI_IMPORTER_H_ */
