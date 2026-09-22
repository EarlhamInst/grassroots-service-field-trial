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
 * submit_plot.c
 *
 *  Created on: 07 Feb 2023
 *      Author: billy
 */


#include "edit_plot.h"

#include "audit.h"

#include "time_util.h"

#include "plot.h"
#include "row.h"
#include "row_jobs.h"

#include "string_parameter.h"
#include "string_array_parameter.h"
#include "time_array_parameter.h"
#include "json_parameter.h"
#include "time_parameter.h"
#include "unsigned_int_parameter.h"
#include "boolean_parameter.h"

#include "dfw_util.h"

#include "observation.h"
#include "standard_row.h"
#include "measured_variable_jobs.h"
#include "phenotype_statistics.h"

#include "study_jobs.h"

/*
 * Static declarations
 */


static NamedParameterType S_ROW_ID = { "RO Id", PT_STRING };

static NamedParameterType S_MEASURED_VARIABLE_NAME  = { "RO Measured Variable Name", PT_STRING };

static NamedParameterType S_PHENOTYPE_RAW_VALUE  = { "RO Phenotype Raw Value", PT_STRING };

static NamedParameterType S_PHENOTYPE_CORRECTED_VALUE = { "RO Phenotype Corrected Value", PT_STRING };

static NamedParameterType S_PHENOTYPE_START_DATE  = { "RO Phenotype Start Date", PT_TIME };

static NamedParameterType S_PHENOTYPE_END_DATE  = { "RO Phenotype End Date", PT_TIME };

static NamedParameterType S_OBSERVATION_NOTES  = { "RO Observation Notes", PT_STRING };

static NamedParameterType S_APPEND_OBSERVATIONS  = { "RO Append Observations", PT_BOOLEAN };


static NamedParameterType S_SAMPLE_INDEX  = { "RO Sample Index", PT_UNSIGNED_INT };


static NamedParameterType S_STUDY_NAME  = { "RO Study Name", PT_STRING };

static NamedParameterType S_STUDY_ID  = { "RO Study Id", PT_STRING };


static NamedParameterType S_ACCESSION  = { "RO Accession", PT_STRING };


static NamedParameterType S_STUDY_INDEX  = { "RO Study Index", PT_UNSIGNED_INT };

static NamedParameterType S_ROW_INDEX  = { "PL Row Index", PT_UNSIGNED_INT };

static NamedParameterType S_COL_INDEX  = { "PL Col Index", PT_UNSIGNED_INT };

static NamedParameterType S_RACK_INDEX  = { "PL Rack", PT_UNSIGNED_INT };

static NamedParameterType S_RACK_NOTES  = { "PL Notes", PT_LARGE_STRING };


static const char *GetPlotEditingServiceName (const Service *service_p);

static const char *GetPlotEditingServiceDescription (const Service *service_p);

static const char *GetPlotEditingServiceAlias (const Service *service_p);

static const char *GetPlotEditingServiceInformationUri (const Service *service_p);

static ParameterSet *GetPlotEditingServiceParameters (Service *service_p, DataResource *resource_p, User *user_p);

static bool GetPlotEditingServiceParameterTypesForNamedParameters (const Service *service_p, const char *param_name_s, ParameterType *pt_p);

static void ReleasePlotEditingServiceParameters (Service *service_p, ParameterSet *params_p);

static ServiceJobSet *RunPlotEditingService (Service *service_p, ParameterSet *param_set_p, User *user_p, ProvidersStateTable *providers_p);


static bool ClosePlotEditingService (Service *service_p);

static ServiceMetadata *GetPlotEditingServiceMetadata (Service *service_p);

static bool AddEditPlotParams (ServiceData *data_p, ParameterSet *param_set_p, DataResource *resource_p);


static const char *GetIdFromResource (DataResource *resource_p, const NamedParameterType plot_param_type);

static bool AddDefaultsFromRow (Row *row_p, ServiceData *data_p, ParameterSet *params_p);

static json_t *GetTableParameterHints (void);

//static bool AddExistingPhenotypeParameters (ParameterSet *params_p, const StandardRow *row_p, FieldTrialServiceData *data_p);

static bool AddPhenotypeParameters (Row *active_row_p, const char *child_group_name_s, ParameterSet *param_set_p, ParameterGroup *parent_group_p, ServiceData *data_p);


static bool AddStringToJSONArray (json_t *array_p, const char *value_s);


static bool AddValidDateToJSONArray (json_t *array_p, const struct tm *time_p);


static bool AddStringToJSONArray (json_t *array_p, const char *value_s);

static bool AddBooleanToJSONArray (json_t *array_p, const bool corrected_flag);

static bool AddStringPhenotypeValueToJSON (json_t *array_p, Observation *observation_p, ObservationValueType ovt);


static bool PopulateExistingValues (Row *active_row_p, char ***existing_mv_names_sss, char ***existing_phenotype_raw_values_sss,
																					char ***existing_phenotype_corrected_values_sss,
																					struct tm ***existing_phenotype_start_dates_ppp, struct tm ***existing_phenotype_end_dates_ppp,
																					char ***existing_observation_notes_sss,
																					uint32 *num_entries_p);

static bool RunForEditPlotParams (FieldTrialServiceData *data_p, ParameterSet *param_set_p, ServiceJob *job_p);


static const struct tm **GetTimeArrayValuesForParameter (ParameterSet *param_set_p, const char *param_s, size_t *num_entries_p);


static OperationStatus ProcessObservations (StandardRow *row_p, ServiceJob *job_p, ParameterSet *param_set_p, FieldTrialServiceData *data_p);

static bool GetObservationParameterTypeForNamedParameter (const char *param_name_s, ParameterType *pt_p);

static Parameter *CreatePlotEditorParameterFromJSON (struct Service *service_p, json_t *param_json_p, const bool concise_flag);


static void SetObservationError (ServiceJob *job_p, const char * const observation_field_s, const void *value_p, void *user_data_p);

static void ReportJSONError (ServiceJob *job_p, const NamedParameterType *param_p, const char * const observation_field_s, const json_t *value_p, const char * const message_s);

/*
 * API definitions
 */


Service *GetPlotEditingService (GrassrootsServer *grassroots_p)
{
	Service *service_p = (Service *) AllocMemory (sizeof (Service));

	if (service_p)
		{
			FieldTrialServiceData *data_p = AllocateFieldTrialServiceData ();

			if (data_p)
				{
					if (InitialiseService (service_p,
														 GetPlotEditingServiceName,
														 GetPlotEditingServiceDescription,
														 GetPlotEditingServiceAlias,
														 GetPlotEditingServiceInformationUri,
														 RunPlotEditingService,
														 NULL,
														 GetPlotEditingServiceParameters,
														 GetPlotEditingServiceParameterTypesForNamedParameters,
														 ReleasePlotEditingServiceParameters,
														 ClosePlotEditingService,
														 NULL,
														 false,
														 SY_SYNCHRONOUS,
														 (ServiceData *) data_p,
														 GetPlotEditingServiceMetadata,
														 NULL,
														 grassroots_p))
						{

							if (ConfigureFieldTrialService (data_p, grassroots_p))
								{
									service_p -> se_custom_parameter_decoder_fn = CreatePlotEditorParameterFromJSON;

									return service_p;
								}

						}		/* if (InitialiseService (.... */

					FreeFieldTrialServiceData (data_p);
				}

			FreeMemory (service_p);
		}		/* if (service_p) */

	return NULL;
}





static const char *GetPlotEditingServiceName (const Service * UNUSED_PARAM (service_p))
{
	return "Edit Field Trial Rack";
}


static const char *GetPlotEditingServiceDescription (const Service * UNUSED_PARAM (service_p))
{
	return "A service to edit an individual field trial rack.";
}


static const char *GetPlotEditingServiceAlias (const Service * UNUSED_PARAM (service_p))
{
	return DFT_GROUP_ALIAS_PREFIX_S SERVICE_GROUP_ALIAS_SEPARATOR "edit_rack";
}


static const char *GetPlotEditingServiceInformationUri (const Service *service_p)
{
	const char *url_s = GetServiceInformationPage (service_p);

	if (!url_s)
		{
			url_s = "https://grassroots.tools/docs/user/services/field_trial/edit_rack.md";
		}

	return url_s;
}



static bool GetPlotEditingServiceParameterTypesForNamedParameters (const Service *service_p, const char *param_name_s, ParameterType *pt_p)
{
	bool b = GetObservationParameterTypeForNamedParameter (param_name_s, pt_p);

	if (!b)
		{
			const NamedParameterType params [] =
				{
					S_ROW_ID,
					S_STUDY_NAME,
					S_STUDY_ID,
					S_STUDY_INDEX,
					S_ROW_INDEX,
					S_COL_INDEX,
					S_RACK_INDEX,
					S_RACK_NOTES,
					S_APPEND_OBSERVATIONS,
					S_ACCESSION,
					{ NULL }
				};

			b = DefaultGetParameterTypeForNamedParameter (param_name_s, pt_p, params);
		}


	return b;
}



static ParameterSet *GetPlotEditingServiceParameters (Service *service_p, DataResource *resource_p, User * UNUSED_PARAM (user_p))
{
	ParameterSet *params_p = AllocateParameterSet ("Plot edit service parameters", "The parameters used for the plot editing service");

	if (params_p)
		{
			ServiceData *data_p = service_p -> se_data_p;

			if (AddEditPlotParams (data_p, params_p, resource_p))
				{
					return params_p;
				}
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "AddEditingFieldTrialParams failed");
				}

			FreeParameterSet (params_p);
		}
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate %s ParameterSet", GetPlotEditingServiceName (service_p));
		}

	return NULL;
}




static void ReleasePlotEditingServiceParameters (Service * UNUSED_PARAM (service_p), ParameterSet *params_p)
{
	FreeParameterSet (params_p);
}




static bool ClosePlotEditingService (Service *service_p)
{
	bool success_flag = true;

	FreeFieldTrialServiceData ((FieldTrialServiceData *) (service_p -> se_data_p));

	return success_flag;
}



static ServiceJobSet *RunPlotEditingService (Service *service_p, ParameterSet *param_set_p, User * UNUSED_PARAM (user_p), ProvidersStateTable * UNUSED_PARAM (providers_p))
{
	FieldTrialServiceData *data_p = (FieldTrialServiceData *) (service_p -> se_data_p);

	service_p -> se_jobs_p = AllocateSimpleServiceJobSet (service_p, NULL, "Submit Plot");

	if (service_p -> se_jobs_p)
		{
			ServiceJob *job_p = GetServiceJobFromServiceJobSet (service_p -> se_jobs_p, 0);

			LogParameterSet (param_set_p, job_p);

			SetServiceJobStatus (job_p, OS_FAILED_TO_START);

			if (!RunForEditPlotParams (data_p, param_set_p, job_p))
				{

				}		/* if (!RunForEditingPlotParams (data_p, param_set_p, job_p)) */


			LogServiceJob (job_p);
		}		/* if (service_p -> se_jobs_p) */

	return service_p -> se_jobs_p;
}


static bool RunForEditPlotParams (FieldTrialServiceData *data_p, ParameterSet *param_set_p, ServiceJob *job_p)
{
	bool success_flag = false;
	const char *row_id_s = NULL;
	OperationStatus status = OS_FAILED;

	if (GetCurrentStringParameterValueFromParameterSet (param_set_p, S_ROW_ID.npt_name_s, &row_id_s))
		{
			Row *active_row_p = GetRowByIdString (row_id_s, VF_CLIENT_FULL, data_p);

			if (active_row_p)
				{
					if (active_row_p -> ro_type == RT_STANDARD)
						{
							StandardRow *active_sr_p = (StandardRow *) active_row_p;
							const char *plot_notes_s = NULL;
							bool append_flag = true;
							const bool *append_flag_p = &append_flag;
							OperationStatus obs_status = OS_IDLE;
							const char *accession_s = NULL;
							bool new_material_flag = false;

							GetCurrentStringParameterValueFromParameterSet (param_set_p, S_ACCESSION.npt_name_s, &accession_s);

							if (!IsStringEmpty (accession_s))
								{

									if (active_sr_p -> sr_planted_material_p)
										{
											/* Check to see if the accession has been updated */

											if (active_sr_p -> sr_planted_material_p -> ma_accession_s)
												{
													if (Stricmp (accession_s, active_sr_p -> sr_planted_material_p -> ma_accession_s))
														{
															/* Accession has changed */
															new_material_flag = true;
														}
												}
											else
												{
													new_material_flag = true;
												}

										}
									else
										{
											new_material_flag = true;
										}

									/*
									 * Do we have a new accession?
									 */
									if (new_material_flag)
										{
											GeneBank *gene_bank_p = NULL;

											/* In the future we could have the gene bank name coming from the service */
											if (!gene_bank_p)
												{
													gene_bank_p = GetGeneBankByName (GENE_BANK_GRU_S, data_p);
												}

											if (gene_bank_p)
												{
													Material *material_p = GetOrCreateMaterialByAccession (accession_s, gene_bank_p, data_p);

													if (material_p)
														{
															if (active_sr_p -> sr_planted_material_p)
																{
																	/*  remove accession */
																	FreeMaterial (active_sr_p -> sr_planted_material_p);
																}

															active_sr_p -> sr_planted_material_p = material_p;
														}
													else
														{
															AddParameterErrorMessageToServiceJob (job_p, S_ACCESSION.npt_name_s, S_ACCESSION.npt_type, "Failed to find material for accession");
															PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to get material for accession \"%s\"", accession_s);
														}

												}
											else
												{
													AddParameterErrorMessageToServiceJob (job_p, S_ACCESSION.npt_name_s, S_ACCESSION.npt_type, "Failed to get GRU Gene Bank for getting accession");
												}

										}
								}
							else
								{

									/*
									if (active_sr_p -> sr_material_p)
										{
											//  remove accession
											FreeMaterial (active_sr_p -> sr_material_p);
											active_sr_p -> sr_material_p = NULL;
										}
									*/
								}



							GetCurrentBooleanParameterValueFromParameterSet (param_set_p, S_APPEND_OBSERVATIONS.npt_name_s, &append_flag_p);

							if (!append_flag)
								{
									/* remove existing observations */
								}

							obs_status = ProcessObservations ((StandardRow *) active_row_p, job_p, param_set_p, data_p);

							GetCurrentStringParameterValueFromParameterSet (param_set_p, S_RACK_NOTES.npt_name_s, &plot_notes_s);

							if ((obs_status == OS_SUCCEEDED) || (obs_status == OS_PARTIALLY_SUCCEEDED) || (obs_status == OS_IDLE) || new_material_flag)
								{
									if (SavePlot (active_row_p -> ro_plot_p, data_p))
										{
											status = OS_SUCCEEDED;

											/* remove the cached study */

											if ((active_row_p -> ro_study_p) && (active_row_p -> ro_study_p -> st_id_p))
												{
													char *id_s = GetBSONOidAsString (active_row_p -> ro_study_p -> st_id_p);

													if (id_s)
														{
															RemoveCachedStudyById (id_s, data_p);
															FreeCopiedString (id_s);
														}
													else
														{
															PrintErrors(STM_LEVEL_WARNING, __FILE__, __LINE__, "Failed to get study id as string for plot " UINT32_FMT " in Study \"%s\"", active_row_p -> ro_by_study_index, active_row_p -> ro_study_p -> st_name_s);
														}

												}



										}
								}
							else
								{
									PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "ProcessObservations () failed");
								}
						}
				}		/* if (active_row_p) */
			else
				{
					char *error_s = ConcatenateVarargsStrings ("Failed to find row with id \"", row_id_s, "\"", NULL);

					if (error_s)
						{
							AddParameterErrorMessageToServiceJob (job_p, S_ROW_ID.npt_name_s, S_ROW_ID.npt_type, error_s);
							FreeCopiedString (error_s);
						}
					else
						{
							AddParameterErrorMessageToServiceJob (job_p, S_ROW_ID.npt_name_s, S_ROW_ID.npt_type, "Failed to find matching row");
						}

					PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Failed to find row with id \"%s\"", row_id_s);
				}

		}		/* if (GetCurrentStringParameterValueFromParameterSet (param_set_p, S_ROW_ID.npt_name_s, &row_id_s)) */
	else
		{
			status = OS_IDLE;
		}

	SetServiceJobStatus (job_p, status);

	return success_flag;
}


static OperationStatus ProcessObservations (StandardRow *row_p, ServiceJob *job_p, ParameterSet *param_set_p, FieldTrialServiceData *data_p)
{
	OperationStatus status = OS_FAILED;
	size_t num_mv_entries;
	const char **mvs_ss = GetStringArrayValuesForParameter (param_set_p, S_MEASURED_VARIABLE_NAME.npt_name_s, &num_mv_entries);
	size_t num_raw_entries;
	const char **raw_values_ss = GetStringArrayValuesForParameter (param_set_p, S_PHENOTYPE_RAW_VALUE.npt_name_s, &num_raw_entries);
	size_t num_corrected_entries;
	const char **corrected_values_ss = GetStringArrayValuesForParameter (param_set_p, S_PHENOTYPE_CORRECTED_VALUE.npt_name_s, &num_corrected_entries);
	size_t num_start_dates;
	const struct tm **start_dates_pp = GetTimeArrayValuesForParameter (param_set_p, S_PHENOTYPE_START_DATE.npt_name_s, &num_start_dates);
	size_t num_end_dates;
	const struct tm **end_dates_pp = GetTimeArrayValuesForParameter (param_set_p, S_PHENOTYPE_END_DATE.npt_name_s, &num_end_dates);
	size_t num_notes;
	const char **notes_ss = GetStringArrayValuesForParameter (param_set_p, S_OBSERVATION_NOTES.npt_name_s, &num_notes);

	if (num_mv_entries == num_raw_entries == num_corrected_entries == num_start_dates == num_end_dates == num_notes)
		{
			if (num_mv_entries > 0)
				{
					size_t num_successes = 0;
					size_t i;
					const char **mv_ss = mvs_ss;
					const char **raw_value_ss = raw_values_ss;
					const char **corrected_value_ss = corrected_values_ss;
					const char **note_ss = notes_ss;
					const struct tm **start_date_pp = start_dates_pp;
					const struct tm **end_date_pp = end_dates_pp;
					const char *key_s = "";
					MEM_FLAG mv_mem = MF_ALREADY_FREED;
					const Study *study_p = row_p -> sr_base.ro_study_p;
					uint32 num_existing_phenotypes = study_p -> st_phenotypes_p -> ll_size;


					ObservationMetadata *metadata_p = AllocateObservationMetadata (NULL, NULL, false, OB_DEFAULT_INDEX);

					if (metadata_p)
						{
							for (i = 0; i < num_mv_entries; ++ i, ++ mv_ss, ++ raw_value_ss, ++ corrected_value_ss, ++ note_ss, ++ start_date_pp, ++ end_date_pp)
								{
									MeasuredVariable *mv_p = GetMeasuredVariableByVariableName (*mv_ss, &mv_mem, data_p);

									if (mv_p)
										{
											json_t *raw_value_p = NULL;
											json_t *corrected_value_p = NULL;

											if (*raw_value_ss)
												{
													raw_value_p = json_string (*raw_value_ss);
												}

											if (*corrected_value_ss)
												{
													corrected_value_p = json_string (*corrected_value_ss);
												}

											if (SetObservationMetadataStartDate (metadata_p, *start_date_pp))
												{
													if (SetObservationMetadataEndDate (metadata_p, *end_date_pp))
														{
															bool free_measured_variable_flag = false;
															OperationStatus obs_status = OS_FAILED;

															metadata_p -> om_index = OB_DEFAULT_INDEX;


															obs_status = AddObservationValueToStandardRowByParts (job_p, row_p, mv_p, metadata_p, key_s,
																																										raw_value_p, corrected_value_p, *note_ss,
																																										&free_measured_variable_flag, SetObservationError, NULL);

															if ((obs_status == OS_SUCCEEDED) || (obs_status == OS_PARTIALLY_SUCCEEDED))
																{
																	++ num_successes;
																}
															else
																{
																	PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "AddObservationValueToStandardRowByParts () failed for " UINT32_FMT
																							" in study \"%s\" with raw value \"%s\", corrected value \"%s\", corrected flag %d, obervation index " UINT32_FMT,
																							row_p -> sr_base.ro_by_study_index, row_p -> sr_base.ro_study_p -> st_name_s, *raw_value_ss, *corrected_value_ss,
																							corrected_value_p, metadata_p -> om_index);
																}


															if ((mv_mem == MF_DEEP_COPY) || (mv_mem == MF_SHALLOW_COPY))
																{
																	//FreeMeasuredVariable (mv_p);
																}

															/* check if the measured variable needs to be added to those referenced by the study */
															if (study_p -> st_phenotypes_p)
																{
																	const char *mv_s = GetMeasuredVariableName (mv_p);
																	PhenotypeStatisticsNode *node_p = (PhenotypeStatisticsNode *) (study_p -> st_phenotypes_p -> ll_head_p);
																	bool new_mv_flag = true;

																	while (node_p && new_mv_flag)
																		{
																			if (strcmp (mv_s, node_p -> psn_measured_variable_name_s) == 0)
																				{
																					new_mv_flag = false;
																				}

																			node_p = (PhenotypeStatisticsNode *) (node_p -> psn_node.ln_next_p);
																		}

																	if (new_mv_flag)
																		{

																		}
																}		/* if (study_p -> st_phenotypes_p) */



														}		/* if (SetObservationMetadataStartDate (metadata_p, *start_date_pp)) */
													else
														{
															PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to set end date for mv " UINT32_FMT, i);
														}

												}		/* if (SetObservationMetadataStartDate (metadata_p, *start_date_pp)) */
											else
												{
													PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to set start date for mv " UINT32_FMT, i);
												}


										}		/* if (mv_p) */
									else
										{
											PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "GetMeasuredVariableByVariableName () failed for \"%s\"", *mv_ss);
										}

								}

							FreeObservationMetadata (metadata_p);
						}		/* if (metadata_p) */


					if (num_successes == num_mv_entries)
						{
							status = OS_SUCCEEDED;
						}
					else if (num_successes > 0)
						{
							status = OS_PARTIALLY_SUCCEEDED;
						}

				}		/* if (num_mv_entries > 0) */

			else
				{
					/* No Observations submitted */
					status = OS_IDLE;
				}

		}		/* if (num_mv_entries == num_raw_entries == num_corrected_entries == num_start_dates == num_end_dates == num_notes) */
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "differing array lengths: num_mv_entries " SIZET_FMT " num_raw_entries " SIZET_FMT
									" num_corrected_entries " SIZET_FMT " num_start_dates " SIZET_FMT " num_end_dates " SIZET_FMT
									" num_notes " SIZET_FMT, num_mv_entries, num_raw_entries, num_corrected_entries, num_start_dates, num_end_dates, num_notes);
		}

		
	return status;
}


static void ReportJSONError (ServiceJob *job_p, const NamedParameterType *param_p, const char * const key_s, const json_t *value_p, const char * const message_s)
{
	bool done_error_message_flag = false;
	const json_t *json_value_p = (const json_t *) value_p;

	if (json_value_p)
		{
			char *value_s = json_dumps (json_value_p, 0);

			if (value_s)
				{
					char *error_s = ConcatenateVarargsStrings (message_s, " \"", value_s, "\"", NULL);

					if (error_s)
						{
							AddParameterErrorMessageToServiceJob (job_p, param_p -> npt_name_s, param_p -> npt_type, error_s);
							FreeCopiedString (error_s);
							done_error_message_flag = true;
						}

					free (value_s);
				}

		}

	if (!done_error_message_flag)
		{
			AddParameterErrorMessageToServiceJob (job_p, param_p -> npt_name_s, param_p -> npt_type, message_s);
		}

}


static void SetObservationError (ServiceJob *job_p, const char * const observation_field_s, const void *value_p, void * UNUSED_PARAM (user_data_p))
{
	if (strcmp (observation_field_s, OB_RAW_VALUE_S) == 0)
		{
			ReportJSONError (job_p, &S_PHENOTYPE_RAW_VALUE, observation_field_s, (const json_t *) value_p, "Failed to set raw value for Observation");
		}
	else if (strcmp (observation_field_s, OB_CORRECTED_VALUE_S) == 0)
		{
			ReportJSONError (job_p, &S_PHENOTYPE_CORRECTED_VALUE, observation_field_s, (const json_t *) value_p, "Failed to set corrected value for Observation");
		}
	else if (strcmp (observation_field_s, OB_NOTES_S) == 0)
		{
			ReportJSONError (job_p, &S_OBSERVATION_NOTES, observation_field_s, (const json_t *) value_p, "Failed to set notes for Observation");
		}
	else
		{
			AddGeneralErrorMessageToServiceJob (job_p, "Failed to process Observation");
		}
}


static const struct tm **GetTimeArrayValuesForParameter (ParameterSet *param_set_p, const char *param_s, size_t *num_entries_p)
{
	const struct tm **values_pp = NULL;
	Parameter *param_p = GetParameterFromParameterSetByName (param_set_p, param_s);

	if (param_p)
		{
			if (IsTimeArrayParameter (param_p))
				{
					TimeArrayParameter *ta_param_p = (TimeArrayParameter *) param_p;

					values_pp = GetTimeArrayParameterCurrentValues (ta_param_p);
					*num_entries_p = GetNumberOfTimeArrayCurrentParameterValues (ta_param_p);
				}
			else if (IsTimeParameter (param_p))
				{
					TimeParameter *ti_param_p = (TimeParameter *) param_p;
					const struct tm *value_p = GetTimeParameterCurrentValue (ti_param_p);
					
					values_pp = &value_p;
					*num_entries_p = 1;									
				}
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Unknown ParameterType for \"%s\" %d", param_s, param_p -> pa_type);
				}
	
		}
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "No Parameter named \"%s\"", param_s);
		}

	return values_pp;
}



static ServiceMetadata *GetPlotEditingServiceMetadata (Service *service_p)
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

							term_url_s = CONTEXT_PREFIX_EXPERIMENTAL_FACTOR_ONTOLOGY_S "EFO_0000651";
							input_p = AllocateSchemaTerm (term_url_s, "phenotype", "The observable form taken by some character (or group of characters) "
																						"in an individual or an organism, excluding pathology and disease. The detectable outward manifestations of a specific genotype.");

							if (input_p)
								{
									if (AddSchemaTermToServiceMetadataInput (metadata_p, input_p))
										{
											/* Genotype */
											term_url_s = CONTEXT_PREFIX_EXPERIMENTAL_FACTOR_ONTOLOGY_S "EFO_0000513";
											input_p = AllocateSchemaTerm (term_url_s, "genotype", "Information, making the distinction between the actual physical material "
												"(e.g. a cell) and the information about the genetic content (genotype).");

											if (input_p)
												{
													if (AddSchemaTermToServiceMetadataInput (metadata_p, input_p))
														{
															return metadata_p;
														}		/* if (AddSchemaTermToServiceMetadataOutput (metadata_p, output_p)) */
													else
														{
															PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add output term %s to service metadata", term_url_s);
															FreeSchemaTerm (input_p);
														}

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





static bool AddEditPlotParams (ServiceData *data_p, ParameterSet *param_set_p, DataResource *resource_p)
{
	FieldTrialServiceData *dfw_data_p = (FieldTrialServiceData *) data_p;
	bool success_flag = false;
	Parameter *param_p = NULL;
	ParameterGroup *group_p = CreateAndAddParameterGroupToParameterSet ("Plot", false, data_p, param_set_p);
	Row *active_row_p = NULL;
	const char *active_id_s = GetIdFromResource (resource_p, S_ROW_ID);


	if (active_id_s)
		{
			active_row_p = GetRowByIdString (active_id_s, VF_CLIENT_FULL, dfw_data_p);
		}


	if ((param_p = EasyCreateAndAddStringParameterToParameterSet (data_p, param_set_p, group_p, S_ROW_ID.npt_type, S_ROW_ID.npt_name_s, "Row ID", "The Row to edit", active_id_s, PL_ALL)) != NULL)
		{
//			param_p -> pa_read_only_flag = true;

			param_p -> pa_refresh_service_flag = true;

			if (AddDefaultsFromRow (active_row_p, data_p, param_set_p))
				{
					const char *study_name_s = NULL;
					char *study_id_s = NULL;
					uint32 *study_index_p = NULL;
					uint32 *row_index_p = NULL;
					uint32 *column_index_p = NULL;
					uint32 *rack_index_p = NULL;
					const char *notes_s = NULL;
					const char *accession_s = NULL;

					if (active_row_p)
						{
							Plot *plot_p = active_row_p -> ro_plot_p;

							if (active_row_p -> ro_study_p)
								{
									study_name_s = active_row_p -> ro_study_p -> st_name_s;
									study_id_s = GetBSONOidAsString (active_row_p -> ro_study_p -> st_id_p);

									if (active_row_p -> ro_type == RT_STANDARD)
										{
											StandardRow *sr_p = (StandardRow *) (active_row_p);

											rack_index_p = & (sr_p -> sr_rack_index);

											if (sr_p -> sr_planted_material_p)
												{
													accession_s = sr_p -> sr_planted_material_p -> ma_accession_s;
												}
										}
								}

							if (plot_p)
								{
									row_index_p = & (plot_p -> pl_row_index);
									column_index_p = & (plot_p -> pl_column_index);
									notes_s = plot_p -> pl_comment_s;
								}

								study_index_p = & (active_row_p -> ro_by_study_index);

						}

					if ((param_p = EasyCreateAndAddStringParameterToParameterSet (data_p, param_set_p, group_p, S_STUDY_NAME.npt_type, S_STUDY_NAME.npt_name_s, "Study", "The Study that contains this plot", study_name_s, PL_ALL)) != NULL)
						{
							param_p -> pa_read_only_flag = true;

							if ((param_p = EasyCreateAndAddStringParameterToParameterSet (data_p, param_set_p, group_p, S_STUDY_ID.npt_type, S_STUDY_ID.npt_name_s, "Study ID", "The Study ID", study_id_s, PL_ALL)) != NULL)
								{
									param_p -> pa_read_only_flag = true;

									if ((param_p = EasyCreateAndAddUnsignedIntParameterToParameterSet (data_p, param_set_p, group_p, S_STUDY_INDEX.npt_name_s, "Study Index", "The index of this plot within the Study", study_index_p, PL_ALL)) != NULL)
										{
											param_p -> pa_read_only_flag = true;

											if ((param_p = EasyCreateAndAddUnsignedIntParameterToParameterSet (data_p, param_set_p, group_p, S_ROW_INDEX.npt_name_s, "Row", "The row of this plot within the Study", row_index_p, PL_ALL)) != NULL)
												{
													param_p -> pa_read_only_flag = true;

													if ((param_p = EasyCreateAndAddUnsignedIntParameterToParameterSet (data_p, param_set_p, group_p, S_COL_INDEX.npt_name_s, "Column", "The column of this plot within the Study", column_index_p, PL_ALL)) != NULL)
														{
															param_p -> pa_read_only_flag = true;

															if ((param_p = EasyCreateAndAddUnsignedIntParameterToParameterSet (data_p, param_set_p, group_p, S_RACK_INDEX.npt_name_s, "Rack", "The rack index of this rack within the Study", rack_index_p, PL_ALL)) != NULL)
																{
																	param_p -> pa_read_only_flag = true;

																	if ((param_p = EasyCreateAndAddStringParameterToParameterSet (data_p, param_set_p, group_p, PT_STRING, S_ACCESSION.npt_name_s, "Accession", "The accession for the line planted in this rack", accession_s, PL_ALL)) != NULL)
																		{
																			if ((param_p = EasyCreateAndAddStringParameterToParameterSet (data_p, param_set_p, group_p, PT_LARGE_STRING, S_RACK_NOTES.npt_name_s, "Notes", "Any notes for this rack", notes_s, PL_ALL)) != NULL)
																				{
																					bool append_flag = true;

																					if ((param_p = EasyCreateAndAddBooleanParameterToParameterSet (data_p, param_set_p, group_p, S_APPEND_OBSERVATIONS.npt_name_s, "Append observations", "Append to existing observations. Setting this to false, will clear any existing observations when you submit.",
																																																				 &append_flag, PL_ALL)) != NULL)
																						{
																							const char *child_group_name_s = "Phenotypes";

																							if (AddPhenotypeParameters (active_row_p, child_group_name_s, param_set_p, group_p, data_p))
																								{
																									success_flag = true;
																								}		/* if (AddPhenotypeParameters (active_plot_p, child_group_name_s, param_set_p, group_p, dfw_data_p)) */
																							else
																								{
																									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "AddPhenotypeParameters () failed");
																								}

																						}
																					else
																						{
																							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add %s parameter", S_APPEND_OBSERVATIONS.npt_name_s);
																						}

																				}		/* if ((param_p = EasyCreateAndAddStringParameterToParameterSet (data_p, param_set_p, group_p, PT_LARGE_STRING, S_NOTES.npt_name_s, "Notes", "Any notes for this rack", notes_s, PL_ALL)) != NULL) */
																			else
																				{
																					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add %s parameter", S_RACK_NOTES.npt_name_s);
																				}


																		}		/*if ((param_p = EasyCreateAndAddStringParameterToParameterSet (data_p, param_set_p, group_p, PT_STRING, S_ACCESSION.npt_name_s, "Accession", "The accession for the line planted in this rack", rack_index_p, PL_ALL)) != NULL)  */
																	else
																		{
																			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add %s parameter", S_ACCESSION.npt_name_s);
																		}



																}		/* if ((param_p = EasyCreateAndAddUnsignedIntParameterToParameterSet (data_p, param_set_p, group_p, S_RACK_INDEX.npt_name_s, "Rack", "The rack index of this rack within the Study", rack_index_p, PL_ALL)) != NULL)*/
															else
																{
																	PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add %s parameter", S_RACK_INDEX.npt_name_s);
																}


														}
													else
														{
															PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add %s parameter", S_COL_INDEX.npt_name_s);
														}

												}
											else
												{
													PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add %s parameter", S_ROW_INDEX.npt_name_s);
												}

										}
									else
										{
											PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add %s parameter", S_STUDY_INDEX.npt_name_s);
										}

								}
							else
								{
									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add %s parameter", S_STUDY_ID.npt_name_s);
								}

						}
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add %s parameter", S_STUDY_NAME.npt_name_s);
						}


					if (study_id_s)
						{
							FreeBSONOidString (study_id_s);
						}

				}		/* if (AddPlotDefaultsFromStudy (active_plot_p, data_p, param_set_p)) */


		}
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add %s parameter", S_ROW_ID.npt_name_s);
		}

	if (active_row_p)
		{
			Plot *plot_p = active_row_p -> ro_plot_p;
			Study *study_p = active_row_p -> ro_study_p;

			if (plot_p)
				{
					FreePlot (plot_p);
				}

			if (study_p)
				{
					FreeStudy (study_p);
				}

		}

	return success_flag;
}



static const char *GetIdFromResource (DataResource *resource_p, const NamedParameterType plot_param_type)
{
	const char *id_s = NULL;

	/*
	 * Have we been set some parameter values to refresh from?
	 */
	if (resource_p && (resource_p -> re_data_p))
		{
			const json_t *param_set_json_p = json_object_get (resource_p -> re_data_p, PARAM_SET_KEY_S);

			if (param_set_json_p)
				{
					json_t *params_json_p = json_object_get (param_set_json_p, PARAM_SET_PARAMS_S);

					if (params_json_p)
						{
							id_s = GetStringDefaultValueFromJSON (plot_param_type.npt_name_s, params_json_p);
						}
				}
		}

	return id_s;
}



static bool AddDefaultsFromRow (Row *row_p, ServiceData *data_p, ParameterSet *params_p)
{
	return true;
}



//static bool AddExistingPhenotypeParameters (ParameterSet *params_p, const StandardRow *row_p, FieldTrialServiceData *data_p)
//{
//	ParameterGroup *group_p = CreateAndAddParameterGroupToParameterSet ("Phenotypes", false, & (data_p -> dftsd_base_data), params_p);
//
//	if (group_p)
//		{
//			Parameter *param_p = NULL;
//			json_t *observations_array_p = NULL;
//
//			if (row_p && (row_p -> sr_observations_p))
//				{
//					if (row_p -> sr_observations_p -> ll_size > 0)
//						{
//							observations_array_p = json_array ();
//
//							if (observations_array_p)
//								{
//									bool success_flag = true;
//									ObservationNode *node_p = ((ObservationNode *) (row_p -> sr_observations_p -> ll_head_p));
//
//									while (node_p && success_flag)
//										{
//											Observation *observation_p = node_p -> on_observation_p;
//											json_t *observation_json_p = GetObservationAsJSON (observation_p, VF_CLIENT_FULL);
//
//											if (observation_json_p)
//												{
//													if (json_array_append_new (observations_array_p, observation_json_p) == 0)
//														{
//															node_p = (ObservationNode *) (node_p -> on_node.ln_next_p);
//														}
//													else
//														{
//															PrintJSONToErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, observation_json_p, "Failed to add observation from row " UINT32_FMT " as JSON", row_p -> sr_base.ro_by_study_index);
//
//															json_decref (observation_json_p);
//
//															success_flag = false;
//														}
//												}		/* if (factors_p) */
//											else
//												{
//													PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "GetObservationAsJSON () failed");
//													success_flag = false;
//												}
//
//										}		/* while (node_p) */
//
//									if (!success_flag)
//										{
//											json_decref (observations_array_p);
//											observations_array_p = NULL;
//										}
//
//								}		/* if (factors_array_p) */
//
//						}		/* if (num_treatments > 1) */
//				}
//
//
//			param_p = EasyCreateAndAddJSONParameterToParameterSet (& (data_p -> dftsd_base_data), params_p, group_p, S_OBSERVATIONS.npt_type, S_OBSERVATIONS.npt_name_s, "Observations", "The current Observations", observations_array_p, PL_ALL);
//
//			if (param_p)
//				{
//					json_t *hints_p = GetTableParameterHints ();
//
//					if (hints_p)
//						{
//							if (AddParameterKeyJSONValuePair (param_p, PA_TABLE_COLUMN_HEADINGS_S, hints_p))
//								{
//									return true;
//								}
//
//							json_decref (hints_p);
//						}
//				}
//
//
//		}
//
//	return false;
//}


static json_t *GetTableParameterHints (void)
{
	/*
	headers_s = ConcatenateVarargsStrings (S_SOWING_TITLE_S, delim_s, S_HARVEST_TITLE_S, delim_s, S_WIDTH_TITLE_S, delim_s, S_LENGTH_TITLE_S, delim_s, S_ROW_TITLE_S, delim_s, S_COLUMN_TITLE_S, delim_s,
																				 PL_REPLICATE_TITLE_S, delim_s, S_RACK_TITLE_S, delim_s, S_MATERIAL_TITLE_S, delim_s, S_TRIAL_DESIGN_TITLE_S, delim_s, S_GROWING_CONDITION_TITLE_S, delim_s, S_TREATMENT_TITLE_S, delim_s, NULL);
	 */
	json_t *hints_p = json_array ();

	if (hints_p)
		{
			if (AddColumnParameterHint (OB_PHENOTYPE_S, "If left blank, then the *Sowing date* specified for the Study will be used.", PT_STRING, false, hints_p))
				{
					if (AddColumnParameterHint (OB_START_DATE_S, "Harvest date of the plot. If this is blank, then the *Harvest date* specified for the Study will be used.", PT_STRING, false, hints_p))
						{
							if (AddColumnParameterHint (OB_END_DATE_S, " If this is blank, then the *Plot width* specified for the Study will be used.", PT_STRING, false, hints_p))
								{
									if (AddColumnParameterHint (OB_RAW_VALUE_S, " If this is blank, then the *Plot height* specified for the Study will be used.", PT_TIME, false, hints_p))
										{
											if (AddColumnParameterHint (OB_CORRECTED_VALUE_S, ".If this is blank, then the *Plot height* specified for the Study will be used.", PT_TIME, false, hints_p))
												{
													return hints_p;
												}
										}
								}
						}
				}

			json_decref (hints_p);
		}

	return NULL;
}




static bool AddValidDateToJSONArray (json_t *array_p, const struct tm *time_p)
{
	bool b = false;

	if (time_p)
		{
			char *time_s = GetTimeAsString (time_p, true, NULL);

			if (time_s)
				{
					if (AddStringToJSONArray (array_p, time_s))
						{
							b = true;
						}

					FreeTimeString (time_s);
				}
		}
	else
		{
			if (json_array_append_new (array_p, json_null ()) == 0)
				{
					b = true;
				}
		}

	return b;
}


static bool AddBooleanToJSONArray (json_t *array_p, const bool corrected_flag)
{
	json_t *corrected_p = NULL;

	if (corrected_flag)
		{
			corrected_p = json_true ();
		}
	else
		{
			corrected_p = json_false ();
		}


	if (corrected_p)
		{
			if (json_array_append_new (array_p, corrected_p) == 0)
				{
					return true;
				}
			else
				{
					PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, array_p, "Failed to append \"%s\"", corrected_flag ? "true" : "false");
				}

			json_decref (corrected_p);
		}
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to create json object for \"%s\"", corrected_flag ? "true" : "false");
		}

	return false;
}


static bool AddStringPhenotypeValueToJSON (json_t *array_p, Observation *observation_p, ObservationValueType ovt)
{
	char *value_s = NULL;
	bool free_value_flag = false;

	if (observation_p -> ob_get_value_as_string_fn (observation_p, ovt, &value_s, &free_value_flag))
		{
			if (value_s)
				{
					if (AddStringToJSONArray (array_p, value_s))
						{
							return true;
						}

					if (free_value_flag)
						{
							FreeCopiedString (value_s);
						}
				}
			else
				{
					if (json_array_append_new (array_p, json_null ()) == 0)
						{
							return true;
						}
				}
		}

	return false;
}


static bool AddStringToJSONArray (json_t *array_p, const char *value_s)
{
	if (value_s)
		{
			json_t *value_p = json_string (value_s);

			if (value_p)
				{
					if (json_array_append_new (array_p, value_p) == 0)
						{
							return true;
						}
					else
						{
							PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, array_p, "Failed to append \"%s\"", value_s);
						}

					json_decref (value_p);
				}
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to create json string for \"%s\"", value_s);
				}

		}
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "input value is null");
		}

	return false;
}


static bool AddStringValueToArray (const Observation *observation_p, ObservationValueType ovt, char **value_ss)
{
	char *value_s = NULL;
	bool free_value_flag = false;
	bool success_flag = false;

	if (observation_p -> ob_get_value_as_string_fn (observation_p, ovt, &value_s, &free_value_flag))
		{
			if (value_s)
				{
					if (free_value_flag)
						{
							*value_ss = value_s;
							success_flag = true;
						}
					else
						{
							char *copied_value_s = EasyCopyToNewString (value_s);

							if (copied_value_s)
								{
									*value_ss = copied_value_s;
									success_flag = true;
								}
							else
								{
									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "EasyCopyToNewString () failed for \"%s\"", value_s);
								}
						}
				}
			else
				{
					*value_ss = NULL;
					success_flag = true;
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "observation value is NULL");
				}
		}
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "failed to get observation value as string");
		}

	return success_flag;
}


static bool PopulateExistingValues (Row *active_row_p, char ***existing_mv_names_sss, char ***existing_phenotype_raw_values_sss,
																					char ***existing_phenotype_corrected_values_sss,
																					struct tm ***existing_phenotype_start_dates_ppp, struct tm ***existing_phenotype_end_dates_ppp,
																					char ***existing_observation_notes_sss,
																					uint32 *num_entries_p)
{
	bool success_flag = false;

	if (active_row_p -> ro_type == RT_STANDARD)
		{
			StandardRow *row_p = (StandardRow *) active_row_p;
			const uint32 num_entries = row_p -> sr_observations_p -> ll_size;

			if (num_entries > 0)
				{
					char **existing_mv_names_ss = (char **) AllocMemoryArray (num_entries, sizeof (char *));

					if (existing_mv_names_ss)
						{
							char **existing_phenotype_raw_values_ss = (char **) AllocMemoryArray (num_entries, sizeof (char *));

							if (existing_phenotype_raw_values_ss)
									{
										struct tm **existing_phenotype_start_dates_pp = (struct tm  **) AllocMemoryArray (num_entries, sizeof (struct tm *));

										if (existing_phenotype_start_dates_pp)
											{
												struct tm **existing_phenotype_end_dates_pp = (struct tm  **) AllocMemoryArray (num_entries, sizeof (struct tm *));

												if (existing_phenotype_end_dates_pp)
													{
														char **existing_phenotype_corrected_values_ss = (char **) AllocMemoryArray (num_entries, sizeof (char *));

														if (existing_phenotype_corrected_values_ss)
															{
																char **existing_notes_ss = (char **) AllocMemoryArray (num_entries, sizeof (char *));

																if (existing_notes_ss)
																	{
																		ObservationNode *obs_node_p = (ObservationNode *) (row_p -> sr_observations_p -> ll_head_p);
																		char **existing_mv_name_ss = existing_mv_names_ss;
																		char **existing_phenotype_raw_value_ss = existing_phenotype_raw_values_ss;
																		struct tm **existing_phenotype_start_date_pp = existing_phenotype_start_dates_pp;
																		struct tm **existing_phenotype_end_date_pp = existing_phenotype_end_dates_pp;
																		char **existing_phenotype_corrected_value_ss = existing_phenotype_corrected_values_ss;
																		char **existing_note_ss = existing_notes_ss;

																		success_flag = true;

																		while (obs_node_p && success_flag)
																			{
																				Observation *observation_p = obs_node_p -> on_observation_p;
																				ObservationMetadata *metadata_p = observation_p -> ob_metadata_p;

																				const char *mv_s = GetMeasuredVariableName (observation_p -> ob_phenotype_p);

																				char *copied_mv_s = EasyCopyToNewString (mv_s);

																				if (copied_mv_s)
																					{
																						*existing_mv_name_ss = copied_mv_s;
																						*existing_phenotype_start_date_pp = metadata_p -> om_start_date_p;
																						*existing_phenotype_end_date_pp = metadata_p -> om_end_date_p;

																						if (AddStringValueToArray (observation_p, OVT_RAW_VALUE, existing_phenotype_raw_value_ss))
																							{
																								if (AddStringValueToArray (observation_p, OVT_CORRECTED_VALUE, existing_phenotype_corrected_value_ss))
																									{
																										obs_node_p = (ObservationNode *) (obs_node_p -> on_node.ln_next_p);

																										++ existing_mv_name_ss;
																										++ existing_phenotype_start_date_pp;
																										++ existing_phenotype_end_date_pp;
																										++ existing_phenotype_raw_value_ss;
																										++ existing_phenotype_corrected_value_ss;
																										++ existing_note_ss;
																									}
																								else
																									{
																										PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "AddStringValueToArray () failed for corrected value");
																										success_flag = false;
																									}

																							}
																						else
																							{
																								PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "AddStringValueToArray () failed for raw value");
																								success_flag = false;
																							}
																					}
																				else
																					{
																						PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "EasyCopyToNewString () failed for \"%s\"", mv_s);
																						success_flag = false;
																					}


																			}		/* while (obs_node_p && loop_flag) */

																		if (success_flag)
																			{
																				*existing_mv_names_sss = existing_mv_names_ss;
																				*existing_phenotype_raw_values_sss = existing_phenotype_raw_values_ss;
																				*existing_phenotype_start_dates_ppp = existing_phenotype_start_dates_pp;
																				*existing_phenotype_end_dates_ppp = existing_phenotype_end_dates_pp;
																				*existing_phenotype_corrected_values_sss = existing_phenotype_corrected_values_ss;
																				*existing_observation_notes_sss = existing_notes_ss;

																				*num_entries_p = num_entries;
																				return true;
																			}		/* if (success_flag) */


																		FreeMemory (existing_notes_ss);
																	}		/* if (existing_notes_ss) */
																else
																	{
																		PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate existing_phenotype_notes");
																	}


																FreeMemory (existing_phenotype_corrected_values_ss);
															}		/* if (existing_phenotype_corrected_p) */
														else
															{
																PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate existing_phenotype_corrected_p");
															}

														FreeMemory (existing_phenotype_end_dates_pp);
													}		/* if (existing_phenotype_end_dates_p) */
												else
													{
														PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate existing_phenotype_end_dates_p");
													}

												FreeMemory (existing_phenotype_start_dates_pp);
											}		/* if (existing_phenotype_start_dates_p) */
										else
											{
												PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate existing_phenotype_start_dates_p");
											}

										FreeMemory (existing_phenotype_raw_values_ss);
									}		/* if (existing_phenotype_values_p) */
								else
									{
										PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate existing_phenotype_raw_values_ss");
									}

							FreeMemory (existing_mv_names_ss);
						}		/* if (existing_mv_names_p) */
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate existing_mv_names_p");
						}


				}		/* if (row_p -> sr_observations_p -> ll_size > 0) */
			else
				{
					*num_entries_p = 0;
					return true;
				}
		}

	return false;
}


static bool AddPhenotypeParameters (Row *active_row_p, const char *child_group_name_s, ParameterSet *param_set_p, ParameterGroup *parent_group_p, ServiceData *data_p)
{
	bool success_flag = false;
	char **existing_mv_names_ss = NULL;
	char **existing_phenotype_raw_values_ss = NULL;
	struct tm **existing_phenotype_start_dates_pp = NULL;
	struct tm **existing_phenotype_end_dates_pp = NULL;
	char **existing_phenotype_corrected_values_ss = NULL;
	char **existing_observation_notes_ss = NULL;
	uint32 num_entries = 0;
	ParameterGroup *child_group_p = CreateAndAddParameterGroupChild (parent_group_p, child_group_name_s, true, false);

	if (child_group_p)
		{
			Parameter *mv_name_p;

			if (active_row_p)
				{
					PopulateExistingValues (active_row_p, &existing_mv_names_ss,
																			&existing_phenotype_raw_values_ss, &existing_phenotype_corrected_values_ss,
																			&existing_phenotype_start_dates_pp, &existing_phenotype_end_dates_pp,
																			&existing_observation_notes_ss,
																			&num_entries);
				}


			if (num_entries > 1)
				{
					mv_name_p = EasyCreateAndAddStringArrayParameterToParameterSet (data_p, param_set_p, child_group_p, S_MEASURED_VARIABLE_NAME.npt_name_s, "Measured Variable Name", "The Name of the Measured Variable to add a phenotype for", existing_mv_names_ss, num_entries, PL_ALL);

					if (mv_name_p)
						{
							Parameter *param_p;

							if ((param_p = EasyCreateAndAddStringArrayParameterToParameterSet (data_p, param_set_p, child_group_p, S_PHENOTYPE_RAW_VALUE.npt_name_s, "Phenotype Raw Value", "The observed phenotypic value", existing_phenotype_raw_values_ss, num_entries, PL_ALL)) != NULL)
								{
									if ((param_p = EasyCreateAndAddStringArrayParameterToParameterSet (data_p, param_set_p, child_group_p, S_PHENOTYPE_CORRECTED_VALUE.npt_name_s, "Phenotype Corrected Value", "Tick this to specify if this Phenotypic value is a correction of a previous raw value", existing_phenotype_corrected_values_ss, num_entries, PL_ALL)) != NULL)
										{
											if ((param_p = EasyCreateAndAddTimeArrayParameterToParameterSet (data_p, param_set_p, child_group_p, S_PHENOTYPE_START_DATE.npt_name_s, "Start Date", "The date when the observation happened/started", existing_phenotype_start_dates_pp, num_entries, PL_ALL)) != NULL)
												{
													if ((param_p = EasyCreateAndAddTimeArrayParameterToParameterSet (data_p, param_set_p, child_group_p, S_PHENOTYPE_END_DATE.npt_name_s, "End Date", "The date when the observation finished if it differs from the start date", existing_phenotype_end_dates_pp, num_entries, PL_ALL)) != NULL)
														{
															if ((param_p = EasyCreateAndAddStringArrayParameterToParameterSet (data_p, param_set_p, child_group_p, S_OBSERVATION_NOTES.npt_name_s, "Observation notes", "Any notes about this Phenotypic observation", existing_observation_notes_ss, num_entries, PL_ALL)) != NULL)
																{
																	success_flag = true;
																}
														}
												}
										}
								}


						}


				}
			else
				{
					Parameter *param_p;
					char *mv_s = NULL;
					char *raw_s = NULL;
					char *corrected_s = NULL;
					struct tm *start_p = NULL;
					struct tm *end_p = NULL;
					char *notes_s = NULL;

					if (num_entries == 1)
						{
							mv_s = *existing_mv_names_ss;
							raw_s = *existing_phenotype_raw_values_ss;
							corrected_s = *existing_phenotype_corrected_values_ss;
							start_p = *existing_phenotype_start_dates_pp;
							end_p = *existing_phenotype_end_dates_pp;
							notes_s = *existing_observation_notes_ss;
						}

					if ((mv_name_p = EasyCreateAndAddStringParameterToParameterSet (data_p, param_set_p, child_group_p, S_MEASURED_VARIABLE_NAME.npt_type, S_MEASURED_VARIABLE_NAME.npt_name_s, "Measured Variable Name", "The Name of the Measured Variable to add a phenotype for", mv_s, PL_ALL)) != NULL)
						{
							if ((param_p = EasyCreateAndAddStringParameterToParameterSet (data_p, param_set_p, child_group_p, S_PHENOTYPE_RAW_VALUE.npt_type, S_PHENOTYPE_RAW_VALUE.npt_name_s, "Phenotype Raw Value", "The observed phenotypic raw value", raw_s, PL_ALL)) != NULL)
								{
									if ((param_p = EasyCreateAndAddStringParameterToParameterSet (data_p, param_set_p, child_group_p, S_PHENOTYPE_CORRECTED_VALUE.npt_type, S_PHENOTYPE_CORRECTED_VALUE.npt_name_s, "Phenotype Corrected Value", "A correction of a previous raw value", corrected_s, PL_ALL)) != NULL)
										{
											if ((param_p = EasyCreateAndAddTimeParameterToParameterSet (data_p, param_set_p, child_group_p, S_PHENOTYPE_START_DATE.npt_name_s, "Start Date", "The date when the observation happened/started", start_p, PL_ALL)) != NULL)
												{
													if ((param_p = EasyCreateAndAddTimeParameterToParameterSet (data_p, param_set_p, child_group_p, S_PHENOTYPE_END_DATE.npt_name_s, "End Date", "The date when the observation finished if it differs from the start date", end_p, PL_ALL)) != NULL)
														{
															if ((param_p = EasyCreateAndAddStringParameterToParameterSet (data_p, param_set_p, child_group_p, S_OBSERVATION_NOTES.npt_type, S_OBSERVATION_NOTES.npt_name_s, "Observation notes", "Any notes about this Phenotypic observation", notes_s, PL_ALL)) != NULL)
																{
																	success_flag = true;
																}
														}
												}
										}
								}
						}
				}


			if (!AddRepeatableParameterGroupLabelParam (child_group_p, mv_name_p))
				{

				}

		}		/* if (child_group_p) */
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to create child parameter group \"%s\"", child_group_name_s);
		}


	if (existing_mv_names_ss)
		{
			FreeStringArray (existing_mv_names_ss, num_entries);
		}

	if (existing_phenotype_raw_values_ss)
		{
			FreeStringArray (existing_phenotype_raw_values_ss, num_entries);
		}

	if (existing_phenotype_start_dates_pp)
		{
			FreeMemory (existing_phenotype_start_dates_pp);
		}

	if (existing_phenotype_end_dates_pp)
		{
			FreeMemory (existing_phenotype_end_dates_pp);
		}

	if (existing_phenotype_corrected_values_ss)
		{
			FreeStringArray (existing_phenotype_corrected_values_ss, num_entries);
		}

	if (existing_observation_notes_ss)
		{
			FreeStringArray (existing_observation_notes_ss, num_entries);
		}


	return success_flag;
}




static Parameter *CreatePlotEditorParameterFromJSON (struct Service *service_p, json_t *param_json_p, const bool concise_flag)
{
	Parameter *param_p = NULL;
	const char *name_s = GetJSONString (param_json_p, PARAM_NAME_S);
	bool done_flag = false;
	ParameterType pt = PT_NUM_TYPES;

	if (GetObservationParameterTypeForNamedParameter (name_s, &pt))
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
											
											case PT_TIME:
												{
													TimeArrayParameter *time_array_param_p = AllocateTimeArrayParameterFromJSON (param_json_p, service_p, concise_flag, NULL);

													if (time_array_param_p)
														{
															param_p = & (time_array_param_p -> tap_base_param);
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
											
											case PT_TIME:
												{
													TimeParameter *time_param_p  = AllocateTimeParameterFromJSON (param_json_p, service_p, concise_flag);

													if (time_param_p)
														{
															param_p = & (time_param_p -> tp_base_param);
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

		}		/* if (GetObservationParameterTypeForNamedParameter (name_s, &pt)) */
	else
		{
		}

	if (!param_p)
		{
			param_p = CreateParameterFromJSON (param_json_p, service_p, concise_flag);
		}

	return param_p;
}





static bool GetObservationParameterTypeForNamedParameter (const char *param_name_s, ParameterType *pt_p)
{
	const NamedParameterType params [] =
	{
		S_MEASURED_VARIABLE_NAME,
		S_PHENOTYPE_RAW_VALUE,
		S_PHENOTYPE_START_DATE,
		S_PHENOTYPE_END_DATE,
		S_PHENOTYPE_CORRECTED_VALUE,
		S_OBSERVATION_NOTES,
		{ NULL }
	};


	return DefaultGetParameterTypeForNamedParameter (param_name_s, pt_p, params);
}
