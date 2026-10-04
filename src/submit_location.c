/*
** Copyright 2014-2018 The Earlham Institute
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
 * submit_location.c
 *
 *  Created on: 6 Apr 2019
 *      Author: billy
 */

#include "submit_location.h"

#include "audit.h"

#include "location_jobs.h"


/*
 * Static declarations
 */



static const char *GetLocationSubmissionServiceName (const Service *service_p);

static const char *GetLocationSubmissionServiceDescription (const Service *service_p);

static const char *GetLocationSubmissionServiceAlias (const Service *service_p);

static const char *GetLocationSubmissionServiceInformationUri (const Service *service_p);

static ParameterSet *GetLocationSubmissionServiceParameters (Service *service_p, DataResource *resource_p, User *user_p);

static bool GetLocationSubmissionServiceParameterTypesForNamedParameters (const  Service *service_p, const char *param_name_s, ParameterType *pt_p);

static void ReleaseLocationSubmissionServiceParameters (Service *service_p, ParameterSet *params_p);

static ServiceJobSet *RunLocationSubmissionService (Service *service_p, ParameterSet *param_set_p, User *user_p, ProvidersStateTable *providers_p);


static bool CloseLocationSubmissionService (Service *service_p);

static ServiceMetadata *GetLocationSubmissionServiceMetadata (Service *service_p);


/*
 * API definitions
 */


Service *GetLocationSubmissionService (GrassrootsServer *grassroots_p)
{
	Service *service_p = (Service *) AllocMemory (sizeof (Service));

	if (service_p)
		{
			FieldTrialServiceData *data_p = AllocateFieldTrialServiceData ();

			if (data_p)
				{
					if (InitialiseService (service_p,
														 GetLocationSubmissionServiceName,
														 GetLocationSubmissionServiceDescription,
														 GetLocationSubmissionServiceAlias,
														 GetLocationSubmissionServiceInformationUri,
														 RunLocationSubmissionService,
														 NULL,
														 GetLocationSubmissionServiceParameters,
														 GetLocationSubmissionServiceParameterTypesForNamedParameters,
														 ReleaseLocationSubmissionServiceParameters,
														 CloseLocationSubmissionService,
														 NULL,
														 false,
														 SY_SYNCHRONOUS,
														 (ServiceData *) data_p,
														 GetLocationSubmissionServiceMetadata,
														 GetLocationIndexingData,
														 grassroots_p))
						{

							if (ConfigureFieldTrialService (data_p, grassroots_p))
								{
									return service_p;
								}

						}		/* if (InitialiseService (.... */

					FreeFieldTrialServiceData (data_p);
				}

			FreeMemory (service_p);
		}		/* if (service_p) */

	return NULL;
}


static const char *GetLocationSubmissionServiceName (const Service * UNUSED_PARAM (service_p))
{
	return "Submit Field Trial Location";
}


static const char *GetLocationSubmissionServiceDescription (const Service * UNUSED_PARAM (service_p))
{
	return "A service to submit field trial locations";
}


static const char *GetLocationSubmissionServiceAlias (const Service * UNUSED_PARAM (service_p))
{
	return DFT_GROUP_ALIAS_PREFIX_S SERVICE_GROUP_ALIAS_SEPARATOR "submit_location";
}


static const char *GetLocationSubmissionServiceInformationUri (const Service *service_p)
{
	const char *url_s = GetServiceInformationPage (service_p);

	if (!url_s)
		{
			url_s = "https://grassroots.tools/docs/user/services/field_trial/submit_location.md";
		}

	return url_s;
}



static bool GetLocationSubmissionServiceParameterTypesForNamedParameters (const  Service *service_p, const char *param_name_s, ParameterType *pt_p)
{
	return GetSubmissionLocationParameterTypeForNamedParameter (param_name_s, pt_p);
}



static ParameterSet *GetLocationSubmissionServiceParameters (Service *service_p, DataResource *resource_p, User * UNUSED_PARAM (user_p))
{
	ParameterSet *params_p = AllocateParameterSet ("Field Trial Location submission service parameters", "The parameters used for the Field Trial Location submission service");

	if (params_p)
		{
			ServiceData *data_p = service_p -> se_data_p;

			if (AddSubmissionLocationParams (data_p, params_p, resource_p))
				{
					return params_p;
				}
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "AddSubmissionFieldTrialParams failed");
				}

			FreeParameterSet (params_p);
		}
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate %s ParameterSet", GetLocationSubmissionServiceName (service_p));
		}

	return NULL;
}






static void ReleaseLocationSubmissionServiceParameters (Service * UNUSED_PARAM (service_p), ParameterSet *params_p)
{
	FreeParameterSet (params_p);
}




static bool CloseLocationSubmissionService (Service *service_p)
{
	bool success_flag = true;

	FreeFieldTrialServiceData ((FieldTrialServiceData *) (service_p -> se_data_p));

	return success_flag;
}



static ServiceJobSet *RunLocationSubmissionService (Service *service_p, ParameterSet *param_set_p, User * UNUSED_PARAM (user_p), ProvidersStateTable * UNUSED_PARAM (providers_p))
{
	FieldTrialServiceData *data_p = (FieldTrialServiceData *) (service_p -> se_data_p);

	service_p -> se_jobs_p = AllocateSimpleServiceJobSet (service_p, NULL, "Submit Location");

	if (service_p -> se_jobs_p)
		{
			ServiceJob *job_p = GetServiceJobFromServiceJobSet (service_p -> se_jobs_p, 0);

			LogParameterSet (param_set_p, job_p);

			SetServiceJobStatus (job_p, OS_FAILED_TO_START);

			if (!RunForSubmissionLocationParams (data_p, param_set_p, job_p))
				{

				}		/* if (!RunForSubmissionLocationParams (data_p, param_set_p, job_p)) */


			LogServiceJob (job_p);
		}		/* if (service_p -> se_jobs_p) */

	return service_p -> se_jobs_p;
}




static ServiceMetadata *GetLocationSubmissionServiceMetadata (Service *service_p)
{
	const char *term_url_s = CONTEXT_PREFIX_EDAM_ONTOLOGY_S "topic_3810";
	SchemaTerm *category_p = AllocateSchemaTerm (term_url_s, "Agricultural science",
		"Multidisciplinary study, research and development within the field of agriculture.");

	if (category_p)
		{
			SchemaTerm *subcategory_p;

			term_url_s = CONTEXT_PREFIX_EDAM_ONTOLOGY_S "operation_3431";
			subcategory_p = AllocateSchemaTerm (term_url_s, "Deposition", "Deposit some data in a database or some other type of repository or software system.");

			if (subcategory_p)
				{
					ServiceMetadata *metadata_p = AllocateServiceMetadata (category_p, subcategory_p);

					if (metadata_p)
						{
							SchemaTerm *input_p;

							/* Place */
							term_url_s = CONTEXT_PREFIX_SCHEMA_ORG_S "Place";
							input_p = AllocateSchemaTerm (term_url_s, "Place", "Entities that have a somewhat fixed, physical extension.");

							if (input_p)
								{
									if (AddSchemaTermToServiceMetadataInput (metadata_p, input_p))
										{
											return metadata_p;
										}		/* if (AddSchemaTermToServiceMetadataInput (metadata_p, input_p)) */
									else
										{
											PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add input term %s to service metadata", term_url_s);
											FreeSchemaTerm (input_p);
										}

								}		/* if (input_p) */
							else
								{
									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate input term %s for service metadata", term_url_s);
								}

							FreeServiceMetadata (metadata_p);
						}		/* if (metadata_p) */
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate service metadata");
						}

				}		/* if (subcategory_p) */
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate sub-category term %s for service metadata", term_url_s);
				}

		}		/* if (category_p) */
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate category term %s for service metadata", term_url_s);
		}

	return NULL;
}

