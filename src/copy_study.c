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
 * copy_study.c
 *
 *  Created on: 22 Jul 2025
 *      Author: billy
 */


#include "copy_study.h"

#include "audit.h"

#include "study_jobs.h"

#include "permissions_editor.h"


#include "boolean_parameter.h"

/*
 * Study parameters
 */

static NamedParameterType S_NAME = { "Name to copy the Study to", PT_STRING };

static NamedParameterType S_SRC_STUDY_ID = { "Study to copy", PT_STRING };


static NamedParameterType S_COPY_TREATMENT_FACTORS = { "Copy Treatment Factors?", PT_BOOLEAN };


static NamedParameterType S_COPY_MEASURED_VARIABLES = { "Copy Measured Variables?", PT_BOOLEAN };


static const char * const S_EMPTY_LIST_OPTION_S = "<empty>";


/*
 * Static declarations
 */


static const char *GetStudyCopyServiceName (const Service *service_p);


static const char *GetStudyCopyServiceDescription (const Service *service_p);

static const char *GetStudyCopyServiceAlias (const Service *service_p);

static const char *GetStudyCopyServiceInformationUri (const Service *service_p);

static ParameterSet *GetStudyCopyServiceParameters (Service *service_p, DataResource *resource_p, User *user_p);

static bool GetStudyCopyServiceParameterTypesForNamedParameters (const Service *service_p, const char *param_name_s, ParameterType *pt_p);

static void ReleaseStudyCopyServiceParameters (Service *service_p, ParameterSet *params_p);

static ServiceJobSet *RunStudyCopyService (Service *service_p, ParameterSet *param_set_p, User *user_p, ProvidersStateTable *providers_p);


static bool CloseStudyCopyService (Service *service_p);

static ServiceMetadata *GetStudyCopyServiceMetadata (Service *service_p);

static Parameter *CreateStudyParameterFromJSON (struct Service *service_p, json_t *param_json_p, const bool concise_flag);


/*
 * API definitions
 */


Service *GetStudyCopyService (GrassrootsServer *grassroots_p)
{
	Service *service_p = (Service *) AllocMemory (sizeof (Service));

	if (service_p)
		{
			FieldTrialServiceData *data_p = AllocateFieldTrialServiceData ();

			if (data_p)
				{
					if (InitialiseService (service_p,
														 GetStudyCopyServiceName,
														 GetStudyCopyServiceDescription,
														 GetStudyCopyServiceAlias,
														 GetStudyCopyServiceInformationUri,
														 RunStudyCopyService,
														 NULL,
														 GetStudyCopyServiceParameters,
														 GetStudyCopyServiceParameterTypesForNamedParameters,
														 ReleaseStudyCopyServiceParameters,
														 CloseStudyCopyService,
														 NULL,
														 false,
														 SY_SYNCHRONOUS,
														 (ServiceData *) data_p,
														 GetStudyCopyServiceMetadata,
														 NULL,
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



static const char *GetStudyCopyServiceName (const Service * UNUSED_PARAM (service_p))
{
	return "Copy Field Trial Study";
}


static const char *GetStudyCopyServiceDescription (const Service * UNUSED_PARAM (service_p))
{
	return "Following the same nomenclature as <a href='https://brapi.docs.apiary.io/'>BrAPI</a>, a Study is a phenotyping experiment "
			"taking place at a single location. One or more Studies can take place within a single Trial. This Service is for copying existing Studies to new ones.";
}


static const char *GetStudyCopyServiceAlias (const Service * UNUSED_PARAM (service_p))
{
	return DFT_GROUP_ALIAS_PREFIX_S SERVICE_GROUP_ALIAS_SEPARATOR "copy_study";
}


static const char *GetStudyCopyServiceInformationUri (const Service *service_p)
{
	const char *url_s = GetServiceInformationPage (service_p);

	if (!url_s)
		{
			url_s = "https://grassroots.tools/docs/user/services/field_trial/copy_study.md";
		}

	return url_s;
}



static bool GetStudyCopyServiceParameterTypesForNamedParameters (const struct Service *service_p, const char *param_name_s, ParameterType *pt_p)
{
	const NamedParameterType params [] =
		{
			S_NAME,
			S_SRC_STUDY_ID,
			S_COPY_TREATMENT_FACTORS,
			S_COPY_MEASURED_VARIABLES,
			NULL
		};

	return DefaultGetParameterTypeForNamedParameter (param_name_s, pt_p, params);
}



static ParameterSet *GetStudyCopyServiceParameters (Service *service_p, DataResource *resource_p, User *user_p)
{
	ParameterSet *params_p = AllocateParameterSet ("FieldTrial submission service parameters", "The parameters used for the FieldTrial submission service");

	if (params_p)
		{
			ServiceData *data_p = service_p -> se_data_p;
			FieldTrialServiceData *dfw_data_p = (FieldTrialServiceData *) data_p;
			bool success_flag = false;
			Parameter *param_p = NULL;
			ParameterGroup *group_p = CreateAndAddParameterGroupToParameterSet ("Study Data", false, data_p, params_p);

			if ((param_p = EasyCreateAndAddStringParameterToParameterSet (data_p, params_p, group_p, S_NAME.npt_type, S_NAME.npt_name_s, "New Study name", "The name to copy the existing Study to", NULL, PL_ALL)) != NULL)
				{
					const char *id_s = S_EMPTY_LIST_OPTION_S;

					param_p = EasyCreateAndAddStringParameterToParameterSet (data_p, params_p, group_p, S_SRC_STUDY_ID.npt_type, S_SRC_STUDY_ID.npt_name_s, "Source Study", "The Study to copy", id_s, PL_ALL);

					if (param_p)
						{
							if (SetUpStudiesListParameter (dfw_data_p, param_p, NULL, true))
								{
									bool b = false;

									if ((param_p = EasyCreateAndAddBooleanParameterToParameterSet (data_p, params_p, group_p, S_COPY_TREATMENT_FACTORS.npt_name_s, "Copy Treatment Factors?", "Do you wish to copy the Treatment Factors?", &b, PL_ALL)) != NULL)
										{
											if ((param_p = EasyCreateAndAddBooleanParameterToParameterSet (data_p, params_p, group_p, S_COPY_MEASURED_VARIABLES.npt_name_s, "Copy Measured Variables?", "Do you wish to copy the Measured Variables?", &b, PL_ADVANCEDs)) != NULL)
												{
													success_flag = true;
												}
										}
								}
						}


					if (success_flag)
						{
							return params_p;
						}

				}
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "AddCopyFieldTrialParams failed");
				}

			FreeParameterSet (params_p);
		}
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate %s ParameterSet", GetStudyCopyServiceName (service_p));
		}

	return NULL;
}






static void ReleaseStudyCopyServiceParameters (Service * UNUSED_PARAM (service_p), ParameterSet *params_p)
{
	FreeParameterSet (params_p);
}




static bool CloseStudyCopyService (Service *service_p)
{
	bool success_flag = true;

	FreeFieldTrialServiceData ((FieldTrialServiceData *) (service_p -> se_data_p));

	return success_flag;
}



static ServiceJobSet *RunStudyCopyService (Service *service_p, ParameterSet *param_set_p, User *user_p, ProvidersStateTable * UNUSED_PARAM (providers_p))
{
	FieldTrialServiceData *data_p = (FieldTrialServiceData *) (service_p -> se_data_p);

	service_p -> se_jobs_p = AllocateSimpleServiceJobSet (service_p, NULL, "Submit Study");

	if (service_p -> se_jobs_p)
		{
			const char *name_s = NULL;
			ServiceJob *job_p = GetServiceJobFromServiceJobSet (service_p -> se_jobs_p, 0);

			LogParameterSet (param_set_p, job_p);

			SetServiceJobStatus (job_p, OS_FAILED_TO_START);

			if (GetCurrentStringParameterValueFromParameterSet (param_set_p, S_NAME.npt_name_s, &name_s))
				{
					if (!IsStringEmpty (name_s))
						{
							const char *id_s = NULL;

							if (GetCurrentStringParameterValueFromParameterSet (param_set_p, S_SRC_STUDY_ID.npt_name_s, &id_s))
								{
									if (!IsStringEmpty (id_s))
										{
											if (strcmp (id_s, S_EMPTY_LIST_OPTION_S) != 0)
												{
													Study *src_study_p = GetStudyByIdString (id_s, VF_STORAGE, data_p);

													if (src_study_p)
														{
															const bool *copy_treatment_factors_flag_p = NULL;
															const bool *copy_measured_variables_flag_p = NULL;
															Study *dest_study_p = NULL;

															GetCurrentBooleanParameterValueFromParameterSet (param_set_p, S_COPY_TREATMENT_FACTORS.npt_name_s, &copy_treatment_factors_flag_p);
															//GetCurrentBooleanParameterValueFromParameterSet (param_set_p, S_COPY_MEASURED_VARIABLES.npt_name_s, &copy_measured_variables_flag_p);


															dest_study_p = CopyStudy (src_study_p, name_s,
																												copy_treatment_factors_flag_p ? *copy_treatment_factors_flag_p : false,
																												copy_measured_variables_flag_p ? *copy_measured_variables_flag_p : false, data_p);

															if (dest_study_p)
																{
																	OperationStatus s = SaveStudy (dest_study_p, job_p, data_p, NULL);

																	/* Did we have any errors saving the study? */
																	if ((s != OS_SUCCEEDED) && (s != OS_PARTIALLY_SUCCEEDED))
																		{

																		}

																	SetServiceJobStatus (job_p, s);

																	FreeStudy (dest_study_p);
																}
															else
																{
																	PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "CopyStudy () failed for existing Study \"%s\" with name \"%s\"", src_study_p -> st_name_s, name_s);
																}

															FreeStudy (src_study_p);
														}
													else
														{
															PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "GetStudyByIdString () failed for id \"%s\"", id_s);
														}
												}
										}

								}

						}
				}

			LogServiceJob (job_p);
		}		/* if (service_p -> se_jobs_p) */

	return service_p -> se_jobs_p;
}


static ServiceMetadata *GetStudyCopyServiceMetadata (Service *service_p)
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


