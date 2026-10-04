/*
** Copyright 2014-2023 The Earlham Institute
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
 * browse_programme_history.c
 *
 *  Created on: 24 Aug 2023
 *      Author: billy
 */


#include "browse_programme_history.h"

#include "dfw_util.h"

#include "audit.h"



#include "programme_jobs.h"
#include "person_jobs.h"
#include "string_parameter.h"
#include "string_array_parameter.h"
#include "time_parameter.h"

/*
 * Static declarations
 */



static const char *GetBrowseProgrammeHistoryServiceName (const Service *service_p);

static const char *GetBrowseProgrammeHistoryServiceDescription (const Service *service_p);

static const char *GetBrowseProgrammeHistoryServiceAlias (const Service *service_p);

static const char *GetBrowseProgrammeHistoryServiceInformationUri (const Service *service_p);

static ParameterSet *GetBrowseProgrammeHistoryServiceParameters (Service *service_p, DataResource *resource_p, User *user_p);

static bool GetBrowseProgrammeHistoryServiceParameterTypesForNamedParameters (const Service *service_p, const char *param_name_s, ParameterType *pt_p);

static void ReleaseBrowseProgrammeHistoryServiceParameters (Service *service_p, ParameterSet *params_p);

static ServiceJobSet *RunBrowseProgrammeHistoryService (Service *service_p, ParameterSet *param_set_p, User *user_p, ProvidersStateTable *providers_p);

static ParameterSet *IsResourceForBrowseProgrammeHistoryService (Service *service_p, DataResource *resource_p, Handler *handler_p);

static bool CloseBrowseProgrammeHistoryService (Service *service_p);

static ServiceMetadata *GetBrowseProgrammeHistoryServiceMetadata (Service *service_p);


static Parameter *CreateSubmitProgrammeParameterFromJSON (struct Service *service_p, json_t *param_json_p, const bool concise_flag);


static bool AddBrowseProgrammeHistoryParams (ServiceData *data_p, ParameterSet *param_set_p, Programme *active_programme_p, const char *original_id_s);


static Programme *GetVersionedProgrammeFromResource (DataResource *resource_p, const NamedParameterType programme_param_type, const char **original_id_ss, FieldTrialServiceData *ft_data_p);


static bool SetUpVersionsParameter (const FieldTrialServiceData *data_p, StringParameter *param_p, const char * const id_s,  const char * const timestamp_s, const FieldTrialDatatype dt);









/*
 * API definitions
 */


Service *GetBrowseProgrammeHistoryService (GrassrootsServer *grassroots_p)
{
	Service *service_p = (Service *) AllocMemory (sizeof (Service));

	if (service_p)
		{
			FieldTrialServiceData *data_p = AllocateFieldTrialServiceData ();

			if (data_p)
				{
					if (InitialiseService (service_p,
														 GetBrowseProgrammeHistoryServiceName,
														 GetBrowseProgrammeHistoryServiceDescription,
														 GetBrowseProgrammeHistoryServiceAlias,
														 GetBrowseProgrammeHistoryServiceInformationUri,
														 RunBrowseProgrammeHistoryService,
														 NULL,
														 GetBrowseProgrammeHistoryServiceParameters,
														 GetBrowseProgrammeHistoryServiceParameterTypesForNamedParameters,
														 ReleaseBrowseProgrammeHistoryServiceParameters,
														 CloseBrowseProgrammeHistoryService,
														 NULL,
														 false,
														 SY_SYNCHRONOUS,
														 (ServiceData *) data_p,
														 GetBrowseProgrammeHistoryServiceMetadata,
														 GetProgrammeIndexingData,
														 grassroots_p))
						{

							if (ConfigureFieldTrialService (data_p, grassroots_p))
								{
									service_p -> se_custom_parameter_decoder_fn = CreateSubmitProgrammeParameterFromJSON;

									return service_p;
								}

						}		/* if (InitialiseService (.... */

					FreeFieldTrialServiceData (data_p);
				}

			FreeMemory (service_p);
		}		/* if (service_p) */

	return NULL;
}


static const char *GetBrowseProgrammeHistoryServiceName (const Service * UNUSED_PARAM (service_p))
{
	return "Browse Programme Revisions";
}


static const char *GetBrowseProgrammeHistoryServiceDescription (const Service * UNUSED_PARAM (service_p))
{
	return "Browse all of the revisions of a given Programme";
}


static const char *GetBrowseProgrammeHistoryServiceAlias (const Service * UNUSED_PARAM (service_p))
{
	return DFT_GROUP_ALIAS_PREFIX_S SERVICE_GROUP_ALIAS_SEPARATOR "browse_programme_history";
}


static const char *GetBrowseProgrammeHistoryServiceInformationUri (const Service *service_p)
{
	const char *url_s = GetServiceInformationPage (service_p);

	if (!url_s)
		{
			url_s = "https://grassroots.tools/docs/user/services/field_programme/browse_programme_history.md";
		}

	return url_s;
}


static bool GetBrowseProgrammeHistoryServiceParameterTypesForNamedParameters (const Service *service_p, const char *param_name_s, ParameterType *pt_p)
{
	bool success_flag = false;
	
	const NamedParameterType params [] =
		{
			FT_TIMESTAMP,
			NULL
		};


	if (DefaultGetParameterTypeForNamedParameter (param_name_s, pt_p, params))
		{
			success_flag = true;
		}
	else
		{
			success_flag = GetSubmissionProgrammeParameterTypeForNamedParameter (param_name_s, pt_p);
		}

	return success_flag;
}



static ParameterSet *GetBrowseProgrammeHistoryServiceParameters (Service *service_p, DataResource *resource_p, User * UNUSED_PARAM (user_p))
{
	ParameterSet *params_p = AllocateParameterSet ("FieldTrial History Browser service parameters", "The parameters used for the Browse Field Trial versions service");

	if (params_p)
		{
			ServiceData *data_p = service_p -> se_data_p;
			FieldTrialServiceData *fts_data_p = (FieldTrialServiceData *) data_p;
			const char *original_id_s = NULL;
			Programme *active_programme_p = GetVersionedProgrammeFromResource (resource_p, PROGRAMME_ID, &original_id_s, fts_data_p);

			if (AddBrowseProgrammeHistoryParams (data_p, params_p, active_programme_p, original_id_s))
				{
					return params_p;
				}
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "AddBrowseProgrammeHistoryParams failed");
				}

			FreeParameterSet (params_p);
		}
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate %s ParameterSet", GetBrowseProgrammeHistoryServiceName (service_p));
		}

	return NULL;
}


/*
static Programme *GetVersionedFieldTrialFromResource (DataResource *resource_p, const NamedParameterType programme_param_type, const char **original_id_ss, FieldTrialServiceData *ft_data_p)
{
	Programme *programme_p = (Programme *) GetVersionedObjectFromResource (resource_p, programme_param_type, original_id_ss, ft_data_p,
																																				 GetVersionedProgramme, GetProgrammeByIdString);

	return programme_p;
}
*/



static bool AddProgrammeVersionsList (Programme *active_programme_p, const char *id_s, ParameterSet *param_set_p, ParameterGroup *group_p, const bool read_only_flag, FieldTrialServiceData *dfw_data_p)
{
	bool success_flag = false;
	ServiceData *data_p = (ServiceData *) dfw_data_p;
	Parameter *param_p = NULL;
	const char *timestamp_s = NULL;

	if ((param_p = EasyCreateAndAddStringParameterToParameterSet (data_p, param_set_p, group_p, FT_TIMESTAMP.npt_type, FT_TIMESTAMP.npt_name_s, "Version", "View Programme revisions", timestamp_s, PL_ALL)) != NULL)
		{
			param_p -> pa_read_only_flag = read_only_flag;

			if ((active_programme_p != NULL) && (active_programme_p -> pr_metadata_p != NULL))
				{
					timestamp_s = active_programme_p -> pr_metadata_p -> me_timestamp_s;
				}


			if (SetUpVersionsParameter (dfw_data_p, (StringParameter *) param_p, id_s, timestamp_s, DFTD_PROGRAMME))
				{
					/*
					 * We want to update all of the values in the form
					 * when a user selects a study from the list so
					 * we need to make the parameter automatically
					 * refresh the values. So we set the
					 * pa_refresh_service_flag to true.
					 */
					param_p -> pa_refresh_service_flag = true;

					success_flag = true;
				}
		}

	return success_flag;
}



static bool SetUpVersionsParameter (const FieldTrialServiceData *data_p, StringParameter *param_p, const char * const id_s,  const char * const timestamp_s, const FieldTrialDatatype dt)
{
	bool success_flag = false;

	json_t *results_p = GetAllJSONVersionsOfObject (id_s, dt, data_p);


	if (results_p)
		{
			if (json_is_array (results_p))
				{
					const size_t num_results = json_array_size (results_p);

					success_flag = true;

					if (num_results > 0)
						{
							if (success_flag)
								{
									bool value_set_flag = false;
									size_t i = 0;
									const char *param_value_s = GetStringParameterDefaultValue (param_p);

									while ((i < num_results) && success_flag)
										{
											json_t *entry_p = json_array_get (results_p, i);
											Programme *programme_p = GetProgrammeFromJSON (entry_p, VF_CLIENT_MINIMAL, data_p);

											if (programme_p)
												{
													const char *value_s = FT_DEFAULT_TIMESTAMP_S;

													if ((programme_p -> pr_metadata_p) && (programme_p -> pr_metadata_p -> me_timestamp_s))
														{
															value_s = programme_p -> pr_metadata_p -> me_timestamp_s;

															if (param_value_s && (strcmp (param_value_s, value_s) == 0))
																{
																	value_set_flag = true;
																}
														}

													if (!CreateAndAddStringParameterOption (param_p, value_s, value_s))
														{
															success_flag = false;
															PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add param option for \"%s\"", value_s);
														}

													FreeProgramme (programme_p);
												}		/* if (programme_p) */
											else
												{
													success_flag = false;
													PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, entry_p, "Failed to get Programme");
												}

											if (success_flag)
												{
													++ i;
												}

										}		/* while ((i < num_results) && success_flag) */

									/*
									 * If the parameter's value isn't on the list, reset it
									 */
									if ((param_value_s != NULL) && (value_set_flag == false))
										{
											PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "param value \"%s\" not on list of existing programmes", param_value_s);
										}

								}		/* if (success_flag) */


						}		/* if (num_results > 0) */
					else
						{
							/* nothing to add */
							success_flag = true;
						}

				}		/* if (json_is_array (results_p)) */

			json_decref (results_p);
		}		/* if (results_p) */


	if (success_flag)
		{
			success_flag = SetStringParameterDefaultValue (param_p, timestamp_s);
			success_flag = SetStringParameterCurrentValue (param_p, timestamp_s);
		}

	return success_flag;
}



static bool AddBrowseProgrammeHistoryParams (ServiceData *data_p, ParameterSet *param_set_p, Programme *active_programme_p, const char *original_id_s)
{
	FieldTrialServiceData *ft_data_p = (FieldTrialServiceData *) data_p;
	bool success_flag = false;
	LinkedList *existing_people_p = NULL;
	ParameterGroup *group_p = CreateAndAddParameterGroupToParameterSet ("Main", false, & (ft_data_p -> dftsd_base_data), param_set_p);
	const bool read_only_flag = true;
	json_t *programmes_p = GetAllProgrammesAsJSON (ft_data_p, true);

	if (programmes_p)
		{
			/*
			 * If we don't have an active programme, use the first one in the json results array
			 */
			if (!active_programme_p)
				{
					if (json_is_array (programmes_p))
						{
							if (json_array_size (programmes_p) > 0)
								{
									json_t *programme_json_p = json_array_get (programmes_p, 0);

									active_programme_p = GetProgrammeFromJSON (programme_json_p, VF_CLIENT_MINIMAL, ft_data_p);

									if (!active_programme_p)
										{
											PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, programme_json_p, "GetProgrammeFromJSON () failed");
										}
								}
						}
				}

			if (active_programme_p)
				{
					char *id_s = NULL;
					const char *id_to_use_s = original_id_s;

					if (!id_to_use_s)
						{
							id_s = GetBSONOidAsString (active_programme_p -> pr_id_p);
							id_to_use_s = id_s;
						}

					if (id_to_use_s)
						{
							if (AddProgrammesListFromJSON (id_to_use_s, programmes_p, param_set_p, group_p, false, false, ft_data_p))
								{

									if (AddProgrammeVersionsList (active_programme_p, id_to_use_s, param_set_p, group_p, false, ft_data_p))
										{

										}

									if (AddProgrammeEditor (active_programme_p, id_to_use_s, param_set_p, read_only_flag, ft_data_p))
										{
											success_flag = true;
										}
								}

							if (id_s)
								{
									FreeBSONOidString (id_s);
								}

						}
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to get id from \"%s\"", active_programme_p -> pr_name_s);
						}



				}

			json_decref (programmes_p);
		}


	return success_flag;
}





static void ReleaseBrowseProgrammeHistoryServiceParameters (Service * UNUSED_PARAM (service_p), ParameterSet *params_p)
{
	FreeParameterSet (params_p);
}




static bool CloseBrowseProgrammeHistoryService (Service *service_p)
{
	bool success_flag = true;

	FreeFieldTrialServiceData ((FieldTrialServiceData *) (service_p -> se_data_p));

	return success_flag;
}



static ServiceJobSet *RunBrowseProgrammeHistoryService (Service *service_p, ParameterSet *param_set_p, User * UNUSED_PARAM (user_p), ProvidersStateTable * UNUSED_PARAM (providers_p))
{
	FieldTrialServiceData *data_p = (FieldTrialServiceData *) (service_p -> se_data_p);

	service_p -> se_jobs_p = AllocateSimpleServiceJobSet (service_p, NULL, "Browse Programme history");

	if (service_p -> se_jobs_p)
		{
			ServiceJob *job_p = GetServiceJobFromServiceJobSet (service_p -> se_jobs_p, 0);

			LogParameterSet (param_set_p, job_p);

			SetServiceJobStatus (job_p, OS_FAILED_TO_START);

	//		if (!RunForBrowseProgrammeHistoryParams (data_p, param_set_p, job_p))
	//			{

	//			}		/* if (!RunForBrowseProgrammeHistoryParams (data_p, param_set_p, job_p)) */


			LogServiceJob (job_p);
		}		/* if (service_p -> se_jobs_p) */

	return service_p -> se_jobs_p;
}


static ServiceMetadata *GetBrowseProgrammeHistoryServiceMetadata (Service *service_p)
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




static ParameterSet *IsResourceForBrowseProgrammeHistoryService (Service * UNUSED_PARAM (service_p), DataResource * UNUSED_PARAM (resource_p), Handler * UNUSED_PARAM (handler_p))
{
	return NULL;
}


static Parameter *CreateSubmitProgrammeParameterFromJSON (struct Service *service_p, json_t *param_json_p, const bool concise_flag)
{
	Parameter *param_p = NULL;
	const char *name_s = GetJSONString (param_json_p, PARAM_NAME_S);
	bool done_flag = false;
	ParameterType pt = PT_NUM_TYPES;

	if (GetPersonParameterTypeForNamedParameter (name_s, &pt))
		{
			if (pt != PT_NUM_TYPES)
				{
					json_t *current_value_p = json_object_get (param_json_p, PARAM_CURRENT_VALUE_S);

					if (current_value_p)
						{
							if (json_is_array (current_value_p))
								{
									switch (pt)
										{
											case PT_STRING:
												{
													StringArrayParameter *string_array_param_p = AllocateStringArrayParameterFromJSON (param_json_p, service_p, concise_flag, NULL);

													if (string_array_param_p)
														{
															param_p = & (string_array_param_p -> sap_base_param);
														}
												}
											break;

											default:
												PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, param_json_p, "Unknown ParameterType %u", pt);
												break;
										}		/* switch (pt) */

								}		/* if (json_is_array (current_value_p)) */
							else
								{
									switch (pt)
										{
											case PT_STRING:
												{
													StringParameter *string_param_p  = AllocateStringParameterFromJSON (param_json_p, service_p, concise_flag, &pt);

													if (string_param_p)
														{
															param_p = & (string_param_p -> sp_base_param);
														}
												}
											break;

											default:
												PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, param_json_p, "Unknown ParameterType %u", pt);
												break;
										}		/* switch (pt) */

								}		/* if (json_is_array (current_value_p)) */

						}		/* if (current_value_p) */

				}		/* if (pt != PT_NUM_TYPES) */
			else
				{
					PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, param_json_p, "Unknown ParameterType %u", pt);
				}

		}		/* if (GetPersonParameterTypeForNamedParameter (name_s, &pt)) */
	else
		{
		}

	if (!param_p)
		{
			param_p = CreateParameterFromJSON (param_json_p, service_p, concise_flag);
		}

	return param_p;
}


static Programme *GetVersionedProgrammeFromResource (DataResource *resource_p, const NamedParameterType programme_param_type, const char **original_id_ss, FieldTrialServiceData *ft_data_p)
{
	void *(*get_versioned_obj_fn) (const char *id_s, const char *timestamp_s, const ViewFormat vf, FieldTrialServiceData *ft_data_p) =
			(void * (*) (const char *, const char *, const ViewFormat, FieldTrialServiceData *)) GetVersionedProgramme;

	void *(*get_obj_by_id_fn) (const char *id_s, const ViewFormat vf, FieldTrialServiceData *ft_data_p) =
			(void *(*) (const char *id_s, const ViewFormat vf, FieldTrialServiceData *ft_data_p)) GetProgrammeByIdString;


	Programme *programme_p = (Programme *) GetVersionedObjectFromResource (resource_p, programme_param_type, original_id_ss, ft_data_p,
																																				 get_versioned_obj_fn, get_obj_by_id_fn);

	return programme_p;
}

