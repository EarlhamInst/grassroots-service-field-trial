/*
 ** Copyright 2014-2016 The Earlham Institute
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
#include <string.h>

#include "jansson.h"


#include "dfw_field_trial_service.h"
#include "dfw_field_trial_service_data.h"
#include "memory_allocations.h"
#include "parameter.h"
#include "service_job.h"
#include "mongodb_tool.h"
#include "string_utils.h"
#include "json_tools.h"
#include "grassroots_server.h"
#include "string_linked_list.h"
#include "math_utils.h"
#include "search_options.h"
#include "time_util.h"
#include "io_utils.h"
#include "audit.h"

#include "search_service.h"
#include "submit_field_trial.h"
#include "submit_gene_bank.h"
#include "submit_location.h"
#include "submit_material.h"
#include "submit_study.h"
#include "submit_measured_variables.h"
#include "submit_crop.h"
#include "edit_plot.h"
#include "submit_plots.h"
#include "submit_program.h"
#include "submit_treatment.h"
#include "submit_treatment_factor.h"
#include "study_manager.h"
#include "browse_programme_history.h"
#include "browse_trial_history.h"
#include "browse_study_history.h"

#include "copy_study.h"

#include "field_trial_jobs.h"
#include "study_jobs.h"
#include "location_jobs.h"
#include "plot_jobs.h"
#include "indexing.h"

#ifdef _DEBUG
#define DFW_FIELD_TRIAL_SERVICE_DEBUG	(STM_LEVEL_FINER)
#else
#define DFW_FIELD_TRIAL_SERVICE_DEBUG	(STM_LEVEL_NONE)
#endif



/*
 * STATIC PROTOTYPES
 */

static Service **AddValidService (Service **service_pp, Service *service_p);

/*
 * API FUNCTIONS
 */


ServicesArray *GetServices (User *user_p, GrassrootsServer *grassroots_p)
{
	uint32 num_services = 0;
	Service *all_in_one_submission_service_p = NULL; // GetDFWFieldTrialSubmissionService (grassroots_p);
	Service *field_trial_submission_service_p = NULL; //GetFieldTrialSubmissionService (grassroots_p);
	Service *search_service_p = GetDFWFieldTrialSearchService (grassroots_p);

	Service *location_submission_service_p = GetLocationSubmissionService (grassroots_p);
	Service *study_submission_service_p = GetStudySubmissionService (grassroots_p);
	Service *gene_bank_submission_service_p = GetGeneBankSubmissionService (grassroots_p);
	Service *material_submission_service_p = NULL; //// GetMaterialSubmissionService (grassroots_p);
	Service *measured_variables_submission_service_p = GetMeasuredVariablesSubmissionService (grassroots_p);
	Service *crop_submission_service_p = GetCropSubmissionService (grassroots_p);
	Service *plots_submission_service_p = GetPlotsSubmissionService (grassroots_p);
	Service *plot_editing_service_p = GetPlotEditingService (grassroots_p);
	Service *phenotypes_submission_service_p = NULL; // GetPhenotypesSubmissionService (grassroots_p);
	Service *indexing_service_p = GetFieldTrialIndexingService (grassroots_p);
	Service *programme_submission_service_p = GetProgrammeSubmissionService (grassroots_p);
	Service *treatments_submission_service_p = GetTreatmentSubmissionService (grassroots_p);
	Service *treatment_factor_submission_service_p = GetTreatmentFactorSubmissionService (grassroots_p);
	Service *study_manager_service_p = GetStudyManagerService (grassroots_p);
	Service *programme_history_browser_service_p = GetBrowseProgrammeHistoryService (grassroots_p);
	Service *trial_history_browser_service_p = GetBrowseTrialHistoryService (grassroots_p);
	Service *study_history_browser_service_p = GetBrowseStudyHistoryService (grassroots_p);
	Service *copy_study_service_p = GetStudyCopyService (grassroots_p);


	if (all_in_one_submission_service_p)
		{
			++ num_services;
		}

	if (field_trial_submission_service_p)
		{
			++ num_services;
		}

	if (search_service_p)
		{
			++ num_services;
		}

	if (location_submission_service_p)
		{
			++ num_services;
		}

	if (study_submission_service_p)
		{
			++ num_services;
		}

	if (gene_bank_submission_service_p)
		{
			++ num_services;
		}

	if (material_submission_service_p)
		{
			++ num_services;
		}

	if (measured_variables_submission_service_p)
		{
			++ num_services;
		}

	if (crop_submission_service_p)
		{
			++ num_services;
		}

	if (plots_submission_service_p)
		{
			++ num_services;
		}

	if (plot_editing_service_p)
		{
			++ num_services;
		}

	if (phenotypes_submission_service_p)
		{
			++ num_services;
		}

	if (indexing_service_p)
		{
			++ num_services;
		}


	if (programme_submission_service_p)
		{
			++ num_services;
		}

	if (treatments_submission_service_p)
		{
			++ num_services;
		}

	if (treatment_factor_submission_service_p)
		{
			++ num_services;
		}

	if (study_manager_service_p)
			{
				++ num_services;
			}

	if (programme_history_browser_service_p)
		{
			++ num_services;
		}


	if (trial_history_browser_service_p)
		{
			++ num_services;
		}


	if (study_history_browser_service_p)
		{
			++ num_services;
		}


	if (copy_study_service_p)
		{
			++ num_services;
		}

	if (num_services)
		{
			ServicesArray *services_p = AllocateServicesArray (num_services);

			if (services_p)
				{
					num_services = 0;
					Service **service_pp = services_p -> sa_services_pp;

					service_pp = AddValidService (service_pp, all_in_one_submission_service_p);
				  service_pp = AddValidService (service_pp, search_service_p);
					service_pp = AddValidService (service_pp, field_trial_submission_service_p);

					service_pp = AddValidService (service_pp, study_submission_service_p);


					service_pp = AddValidService (service_pp, location_submission_service_p);

					service_pp = AddValidService (service_pp, gene_bank_submission_service_p);
					service_pp = AddValidService (service_pp, material_submission_service_p);
					service_pp = AddValidService (service_pp, plots_submission_service_p);
					service_pp = AddValidService (service_pp, plot_editing_service_p);
					service_pp = AddValidService (service_pp, measured_variables_submission_service_p);
					service_pp = AddValidService (service_pp, crop_submission_service_p);
					service_pp = AddValidService (service_pp, phenotypes_submission_service_p);
					service_pp = AddValidService (service_pp, indexing_service_p);
					service_pp = AddValidService (service_pp, programme_submission_service_p);
					service_pp = AddValidService (service_pp, treatments_submission_service_p);
					service_pp = AddValidService (service_pp, treatment_factor_submission_service_p);
					service_pp = AddValidService (service_pp, study_manager_service_p);
					service_pp = AddValidService (service_pp, programme_history_browser_service_p);
					service_pp = AddValidService (service_pp, trial_history_browser_service_p);
					service_pp = AddValidService (service_pp, study_history_browser_service_p);
					service_pp = AddValidService (service_pp, copy_study_service_p);


					return services_p;
				}
		}

	if (all_in_one_submission_service_p)
		{
			FreeService (all_in_one_submission_service_p);
		}


	if (search_service_p)
		{
			FreeService (search_service_p);
		}


	if (field_trial_submission_service_p)
		{
			FreeService (field_trial_submission_service_p);
		}


	if (study_submission_service_p)
		{
			FreeService (study_submission_service_p);
		}

	if (location_submission_service_p)
		{
			FreeService (location_submission_service_p);
		}


	if (gene_bank_submission_service_p)
		{
			FreeService (gene_bank_submission_service_p);
		}

	if (material_submission_service_p)
		{
			FreeService (material_submission_service_p);
		}

	if (plots_submission_service_p)
		{
			FreeService (plots_submission_service_p);
		}

	if (plot_editing_service_p)
		{
			FreeService (plot_editing_service_p);
		}

	if (measured_variables_submission_service_p)
		{
			FreeService (measured_variables_submission_service_p);
		}

	if (crop_submission_service_p)
		{
			FreeService (crop_submission_service_p);
		}

	if (phenotypes_submission_service_p)
		{
			FreeService (phenotypes_submission_service_p);
		}

	if (indexing_service_p)
		{
			FreeService (indexing_service_p);
		}

	if (programme_submission_service_p)
		{
			FreeService (programme_submission_service_p);
		}

	if (treatments_submission_service_p)
		{
			FreeService (treatments_submission_service_p);
		}

	if (treatment_factor_submission_service_p)
		{
			FreeService (treatment_factor_submission_service_p);
		}

	if (study_manager_service_p)
		{
			FreeService (study_manager_service_p);
		}

	if (programme_history_browser_service_p)
		{
			FreeService (trial_history_browser_service_p);
		}

	if (trial_history_browser_service_p)
		{
			FreeService (trial_history_browser_service_p);
		}

	if (study_history_browser_service_p)
		{
			FreeService (trial_history_browser_service_p);
		}

	if (copy_study_service_p)
		{
			FreeService (copy_study_service_p);
		}


	return NULL;
}


void ReleaseServices (ServicesArray *services_p)
{
	FreeServicesArray (services_p);
}




/*
bool AddErrorMessage (json_t *errors_p, const json_t *values_p, const size_t row, const char * const error_s)
{
	bool success_flag = false;
	const char *pathogenomics_id_s = GetJSONString (values_p, PG_ID_S);

	if (pathogenomics_id_s)
		{
			json_error_t error;
			json_t *error_p = json_pack_ex (&error, 0, "{s:s,s:i,s:s}", "ID", pathogenomics_id_s, "row", row, "error", error_s);

			if (error_p)
				{
					if (json_array_append_new (errors_p, error_p) == 0)
						{
							success_flag = true;
						}
				}
		}

#if DFW_FIELD_TRIAL_SERVICE_DEBUG >= STM_LEVEL_FINE
	PrintJSONToLog (STM_LEVEL_FINE, __FILE__, __LINE__, errors_p, "errors data: ");
#endif

	return success_flag;
}
*/





static Service **AddValidService (Service **service_pp, Service *service_p)
{
	if (service_p)
		{
			*service_pp = service_p;
			++ service_pp;
		}

	return service_pp;
}
