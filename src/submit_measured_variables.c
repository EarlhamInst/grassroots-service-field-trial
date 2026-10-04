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
 * submit_treatments.c
 *
 *  Created on: 10 Apr 2019
 *      Author: billy
 */




#include "measured_variable_jobs.h"
#include "submit_measured_variables.h"

#include "audit.h"



/*
 * Static declarations
 */



static const char *GetMeasuredVariableSubmissionServiceName (const Service *service_p);

static const char *GetMeasuredVariableSubmissionServiceDescription (const Service *service_p);

static const char *GetMeasuredVariableSubmissionServiceAlias (const Service *service_p);

static const char *GetMeasuredVariableSubmissionServiceInformationUri (const Service *service_p);

static ParameterSet *GetMeasuredVariableSubmissionServiceParameters (Service *service_p, DataResource *resource_p, User *user_p);

static bool GetMeasuredVariableSubmissionServiceParameterTypesForNamedParameters (const Service *service_p, const char *param_name_s, ParameterType *pt_p);

static void ReleaseMeasuredVariableSubmissionServiceParameters (Service *service_p, ParameterSet *params_p);

static ServiceJobSet *RunMeasuredVariableSubmissionService (Service *service_p, ParameterSet *param_set_p, User *user_p, ProvidersStateTable *providers_p);


static bool CloseMeasuredVariableSubmissionService (Service *service_p);

static ServiceMetadata *GetMeasuredVariableSubmissionServiceMetadata (Service *service_p);


/*
 * API definitions
 */


Service *GetMeasuredVariablesSubmissionService (GrassrootsServer *grassroots_p)
{
	Service *service_p = (Service *) AllocMemory (sizeof (Service));

	if (service_p)
		{
			FieldTrialServiceData *data_p = AllocateFieldTrialServiceData ();

			if (data_p)
				{
					if (InitialiseService (service_p,
														 GetMeasuredVariableSubmissionServiceName,
														 GetMeasuredVariableSubmissionServiceDescription,
														 GetMeasuredVariableSubmissionServiceAlias,
														 GetMeasuredVariableSubmissionServiceInformationUri,
														 RunMeasuredVariableSubmissionService,
														 NULL,
														 GetMeasuredVariableSubmissionServiceParameters,
														 GetMeasuredVariableSubmissionServiceParameterTypesForNamedParameters,
														 ReleaseMeasuredVariableSubmissionServiceParameters,
														 CloseMeasuredVariableSubmissionService,
														 NULL,
														 false,
														 SY_SYNCHRONOUS,
														 (ServiceData *) data_p,
														 GetMeasuredVariableSubmissionServiceMetadata,
														 GetMeasuredVariableIndexingData,
														 grassroots_p))
						{

							if (ConfigureFieldTrialService (data_p, grassroots_p))
								{
									return service_p;
								}

						}		/* if (InitialiseService (.... */
					else
						{
							if (!service_p -> se_data_p)
								{
									FreeFieldTrialServiceData (data_p);
								}
						}

				}		/* if (data_p) */


			FreeService (service_p);
		}		/* if (service_p) */

	return NULL;
}


static const char *GetMeasuredVariableSubmissionServiceName (const Service * UNUSED_PARAM (service_p))
{
	return "Submit Field Trial Measured Variables";
}


static const char *GetMeasuredVariableSubmissionServiceDescription (const Service * UNUSED_PARAM (service_p))
{
	return "A service to submit field trial measured variables";
}


static const char *GetMeasuredVariableSubmissionServiceAlias (const Service * UNUSED_PARAM (service_p))
{
	return DFT_GROUP_ALIAS_PREFIX_S SERVICE_GROUP_ALIAS_SEPARATOR "submit_measured_variables";
}


static const char *GetMeasuredVariableSubmissionServiceInformationUri (const Service *service_p)
{
	const char *url_s = GetServiceInformationPage (service_p);

	return url_s;
}



static bool GetMeasuredVariableSubmissionServiceParameterTypesForNamedParameters (const Service *service_p, const char *param_name_s, ParameterType *pt_p)
{
	return GetSubmissionMeasuredVariableParameterTypeForNamedParameter (param_name_s, pt_p);
}



static ParameterSet *GetMeasuredVariableSubmissionServiceParameters (Service *service_p, DataResource * UNUSED_PARAM (resource_p), User * UNUSED_PARAM (user_p))
{
	ParameterSet *params_p = AllocateParameterSet ("FieldTrial submission service parameters", "The parameters used for the FieldTrial submission service");

	if (params_p)
		{
			ServiceData *data_p = service_p -> se_data_p;

			if (AddSubmissionMeasuredVariableParams (data_p, params_p))
				{
					return params_p;
				}
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "AddSubmissionMeasuredVariableParams failed");
				}

			FreeParameterSet (params_p);
		}
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate %s ParameterSet", GetMeasuredVariableSubmissionServiceName (service_p));
		}

	return NULL;
}






static void ReleaseMeasuredVariableSubmissionServiceParameters (Service * UNUSED_PARAM (service_p), ParameterSet *params_p)
{
	FreeParameterSet (params_p);
}




static bool CloseMeasuredVariableSubmissionService (Service *service_p)
{
	bool success_flag = true;

	FreeFieldTrialServiceData ((FieldTrialServiceData *) (service_p -> se_data_p));

	return success_flag;
}



static ServiceJobSet *RunMeasuredVariableSubmissionService (Service *service_p, ParameterSet *param_set_p, User * UNUSED_PARAM (user_p), ProvidersStateTable * UNUSED_PARAM (providers_p))
{
	FieldTrialServiceData *data_p = (FieldTrialServiceData *) (service_p -> se_data_p);

	service_p -> se_jobs_p = AllocateSimpleServiceJobSet (service_p, NULL, "Submit MeasuredVariable");

	if (service_p -> se_jobs_p)
		{
			ServiceJob *job_p = GetServiceJobFromServiceJobSet (service_p -> se_jobs_p, 0);

			LogParameterSet (param_set_p, job_p);

			SetServiceJobStatus (job_p, OS_FAILED_TO_START);

			if (param_set_p)
				{
					if (param_set_p -> ps_current_level == PL_ADVANCED)
						{
							RunForCropOntologyAPIImport (param_set_p, job_p, data_p);
						}
					else
						{
							if (!RunForSubmittedSpreadsheet (data_p, param_set_p, job_p))
								{

								}		/* if (!RunForSubmittedSpreadsheet (data_p, param_set_p, job_p)) */

						}

				}		/* if (param_set_p) */

			LogServiceJob (job_p);
		}		/* if (service_p -> se_jobs_p) */

	return service_p -> se_jobs_p;
}


static ServiceMetadata *GetMeasuredVariableSubmissionServiceMetadata (Service *service_p)
{
	const char *term_url_s = CONTEXT_PREFIX_EDAM_ONTOLOGY_S "topic_0625";
	SchemaTerm *category_p = AllocateSchemaTerm (term_url_s, "Genotype and phenotype",
		"The study of genetic constitution of a living entity, such as an individual, and organism, a cell and so on, "
		"typically with respect to a particular observable phenotypic traits, or resources concerning such traits, which "
		"might be an aspect of biochemistry, physiology, morphology, anatomy, development and so on.");

	if (category_p)
		{
			SchemaTerm *subcategory_p;

			term_url_s = CONTEXT_PREFIX_EDAM_ONTOLOGY_S "operation_0304";
			subcategory_p = AllocateSchemaTerm (term_url_s, "Query and retrieval", "Search or query a data resource and retrieve entries and / or annotation.");

			if (subcategory_p)
				{
					ServiceMetadata *metadata_p = AllocateServiceMetadata (category_p, subcategory_p);

					if (metadata_p)
						{
							SchemaTerm *input_p;

							term_url_s = CONTEXT_PREFIX_EDAM_ONTOLOGY_S "data_0968";
							input_p = AllocateSchemaTerm (term_url_s, "Keyword",
								"Boolean operators (AND, OR and NOT) and wildcard characters may be allowed. Keyword(s) or phrase(s) used (typically) for text-searching purposes.");

							if (input_p)
								{
									if (AddSchemaTermToServiceMetadataInput (metadata_p, input_p))
										{
											SchemaTerm *output_p;

											/* Place */
											term_url_s = CONTEXT_PREFIX_SCHEMA_ORG_S "Place";
											output_p = AllocateSchemaTerm (term_url_s, "Place", "Entities that have a somewhat fixed, physical extension.");

											if (output_p)
												{
													if (AddSchemaTermToServiceMetadataOutput (metadata_p, output_p))
														{
															/* Date */
															term_url_s = CONTEXT_PREFIX_SCHEMA_ORG_S "Date";
															output_p = AllocateSchemaTerm (term_url_s, "Date", "A date value in ISO 8601 date format.");

															if (output_p)
																{
																	if (AddSchemaTermToServiceMetadataOutput (metadata_p, output_p))
																		{
																			/* Pathogen */
																			term_url_s = CONTEXT_PREFIX_EXPERIMENTAL_FACTOR_ONTOLOGY_S "EFO_0000643";
																			output_p = AllocateSchemaTerm (term_url_s, "pathogen", "A biological agent that causes disease or illness to its host.");

																			if (output_p)
																				{
																					if (AddSchemaTermToServiceMetadataOutput (metadata_p, output_p))
																						{
																							/* Phenotype */
																							term_url_s = CONTEXT_PREFIX_EXPERIMENTAL_FACTOR_ONTOLOGY_S "EFO_0000651";
																							output_p = AllocateSchemaTerm (term_url_s, "phenotype", "The observable form taken by some character (or group of characters) "
																								"in an individual or an organism, excluding pathology and disease. The detectable outward manifestations of a specific genotype.");

																							if (output_p)
																								{
																									if (AddSchemaTermToServiceMetadataOutput (metadata_p, output_p))
																										{
																											/* Genotype */
																											term_url_s = CONTEXT_PREFIX_EXPERIMENTAL_FACTOR_ONTOLOGY_S "EFO_0000513";
																											output_p = AllocateSchemaTerm (term_url_s, "genotype", "Information, making the distinction between the actual physical material "
																												"(e.g. a cell) and the information about the genetic content (genotype).");

																											if (output_p)
																												{
																													if (AddSchemaTermToServiceMetadataOutput (metadata_p, output_p))
																														{
																															return metadata_p;
																														}		/* if (AddSchemaTermToServiceMetadataOutput (metadata_p, output_p)) */
																													else
																														{
																															PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add output term %s to service metadata", term_url_s);
																															FreeSchemaTerm (output_p);
																														}

																												}		/* if (output_p) */
																											else
																												{
																													PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate output term %s for service metadata", term_url_s);
																												}
																										}		/* if (AddSchemaTermToServiceMetadataOutput (metadata_p, output_p)) */
																									else
																										{
																											PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add output term %s to service metadata", term_url_s);
																											FreeSchemaTerm (output_p);
																										}

																								}		/* if (output_p) */
																							else
																								{
																									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate output term %s for service metadata", term_url_s);
																								}

																						}		/* if (AddSchemaTermToServiceMetadataOutput (metadata_p, output_p)) */
																					else
																						{
																							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add output term %s to service metadata", term_url_s);
																							FreeSchemaTerm (output_p);
																						}

																				}		/* if (output_p) */
																			else
																				{
																					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate output term %s for service metadata", term_url_s);
																				}

																		}		/* if (AddSchemaTermToServiceMetadataOutput (metadata_p, output_p)) */
																	else
																		{
																			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add output term %s to service metadata", term_url_s);
																			FreeSchemaTerm (output_p);
																		}

																}		/* if (output_p) */
															else
																{
																	PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate output term %s for service metadata", term_url_s);
																}


														}		/* if (AddSchemaTermToServiceMetadataOutput (metadata_p, output_p)) */
													else
														{
															PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add output term %s to service metadata", term_url_s);
															FreeSchemaTerm (output_p);
														}

												}		/* if (output_p) */
											else
												{
													PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate output term %s for service metadata", term_url_s);
												}

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


