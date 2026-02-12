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
 * row_jobs.c
 *
 *  Created on: 28 Oct 2018
 *      Author: billy
 */
#include "row_jobs.h"
#include "plot_jobs.h"
#include "measured_variable_jobs.h"
#include "dfw_util.h"
#include "string_utils.h"
#include "study_jobs.h"
#include "math_utils.h"
#include "time_util.h"
#include "observation.h"
#include "treatment.h"
#include "treatment_factor.h"
#include "treatment_jobs.h"
#include "treatment_factor_value.h"


#include "char_parameter.h"
#include "json_parameter.h"
#include "string_parameter.h"

#include "frictionless_data_util.h"

#include "observation_metadata.h"

/*
 * static declarations
 */
static const char * const S_RACK_S = "Rack";
static const char * const S_PLOT_INDEX_S = "Plot index";


typedef struct 
{
	uint32 oe_row;
	const char *oe_column_s;
} ObservationError;


/**
 * Report any errors from GetObservationMetadata ()
 */


static void SetObservationError (ServiceJob *job_p, const char * const observation_field_s, const void *value_p, void *user_data_p);


/*
 * API Definitions
 */


bool AddRowFrictionlessDataDetails (const Row *row_p, json_t *row_fd_p, const FieldTrialServiceData *service_data_p, const char * const null_sequence_s)
{
	bool success_flag = false;

	if (SetJSONInteger (row_fd_p, PL_INDEX_TABLE_TITLE_S, row_p -> ro_by_study_index))
		{
			if (row_p -> ro_add_to_fd_fn)
				{
					success_flag = row_p -> ro_add_to_fd_fn (row_p, row_fd_p, service_data_p, null_sequence_s);
				}
			else
				{
					success_flag = true;
				}

			switch (row_p -> ro_type)
			{
				case RT_STANDARD:
					{

					}
					break;

				case RT_BLANK:
					{
						if (SetJSONBoolean (row_fd_p, RO_BLANK_S, true))
							{
								success_flag = true;
							}
						else
							{
								PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, row_fd_p, "Failed to add \"%s\": true", RO_BLANK_S);
							}
					}
					break;

				case RT_DISCARD:
					{
						if (SetJSONBoolean (row_fd_p, RO_DISCARD_S, true))
							{
								success_flag = true;
							}
						else
							{
								PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, row_fd_p, "Failed to add \"%s\": true", RO_DISCARD_S);
							}
					}
					break;

				default:
					{
						PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "AddRowFrictionlessDataDetails () failed: unknown type %d", row_p -> ro_type);
					}
					break;

			}		/* switch (row_p -> ro_type) */

		}		/* if (SetJSONInteger (row_fd_p, PL_INDEX_TABLE_TITLE_S, row_p -> ro_by_study_index)) */
	else
		{
			PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, row_fd_p, "Failed to add \"%s\": " UINT32_FMT, PL_INDEX_TABLE_TITLE_S, row_p -> ro_by_study_index);
		}


	return success_flag;
}




/*
 * static definitions
 */




OperationStatus AddTreatmentFactorValuesToStandardRow (StandardRow *row_p, json_t *plot_json_p, Study *study_p, FieldTrialServiceData *data_p)
{
	OperationStatus status = OS_IDLE;
	void *temp_p = NULL;
	const char *key_s;
	json_t *value_p;
	size_t num_treatments = 0;
	size_t num_added = 0;

	json_object_foreach_safe (plot_json_p, temp_p, key_s, value_p)
	{
		/* Is it a treatment? */
		Treatment *treatment_p = GetTreatmentByURL (key_s, VF_STORAGE, data_p);


		if (treatment_p)
			{
				/*
				 * Does the Study have a TreatmentFactor for this
				 * Treatment?
				 */
				TreatmentFactor *tf_p = GetTreatmentFactorForStudy (study_p, treatment_p -> tr_id_p, data_p);

				if (tf_p)
					{
						if (json_is_string (value_p))
							{
								const char *name_s = json_string_value (value_p);
								const char *value_s = GetTreatmentFactorValue (tf_p, name_s);

								/* Is it a valid defined label? */
								if (value_s)
									{
										if (AddTreatmentFactorValueToRowByParts (row_p, tf_p, name_s))
											{
												++ num_added;
											}
									}
							}


						//FreeTreatmentFactor (tf_p);
					}

				FreeTreatment (treatment_p);

				/*
				 * We know that it's a Treatment so remove it from any later processing
				 * and increment the number that we've seen
				 */
				json_object_del (plot_json_p, key_s);
				++ num_treatments;

			}		/* if (treatment_p) */

	}		/* json_array_foreach_safe (plot_json_p, temp_p, key_s, value_p) */

	if (num_treatments > 0)
		{
			if (num_added == num_treatments)
				{
					status = OS_SUCCEEDED;
				}
			else if (num_added > 0)
				{
					status = OS_PARTIALLY_SUCCEEDED;
				}
			else
				{
					status = OS_FAILED;
				}
		}

	return status;
}



//OperationStatus OldAddObservationValuesToBaseRow (BaseRow *row_p, json_t *observation_json_p, Study *study_p, const FieldTrialServiceData *data_p)
//{
//	OperationStatus status = OS_FAILED;
//
//	bool loop_success_flag = true;
//	void *iterator_p = json_object_iter (observation_json_p);
//	size_t imported_obs = 0;
//	size_t total_obs = 0;
//	LinkedList *processed_keys_p = AllocateStringLinkedList ();
//
//	if (processed_keys_p)
//		{
//			while (iterator_p && loop_success_flag)
//				{
//					const char *key_s = json_object_iter_key (iterator_p);
//					json_t *value_p = json_object_iter_value (iterator_p);
//
//					/*
//					 * ignore our column names
//					 */
//					if ((strcmp (key_s, S_PLOT_INDEX_S) != 0) && (strcmp (key_s, S_RACK_S) != 0))
//						{
//							/*
//							 * make sure it isn't a date column
//							 */
//							const char * const DATE_ENDING_S = " date";
//							const char * const CORRECTED_ENDING_S = " corrected";
//
//							if ((!DoesStringEndWith (key_s, DATE_ENDING_S)) && (!DoesStringEndWith (key_s, CORRECTED_ENDING_S)))
//								{
//									MeasuredVariable *measured_variable_p = GetMeasuredVariableByVariableName (key_s, data_p);
//
//									if (measured_variable_p)
//										{
//											Observation *observation_p = NULL;
//											bool added_phenotype_flag = false;
//											const char *raw_value_s = json_string_value (value_p);
//											const char *corrected_value_s = NULL;
//											char *column_header_s = NULL;
//
//											/* corrected value */
//											column_header_s = ConcatenateStrings (key_s, CORRECTED_ENDING_S);
//											if (column_header_s)
//												{
//													corrected_value_s = GetJSONString (observation_json_p, column_header_s);
//													FreeCopiedString (column_header_s);
//												}		/* if (column_header_s) */
//
//
//											if ((!IsStringEmpty (raw_value_s)) || (!IsStringEmpty (corrected_value_s)))
//												{
//													const char *growth_stage_s = NULL;
//													const char *method_s = NULL;
//													ObservationNature nature = ON_ROW;
//													Instrument *instrument_p = NULL;
//													bson_oid_t *observation_id_p = GetNewBSONOid ();
//													struct tm *observation_date_p = NULL;
//
//													++ total_obs;
//
//													/*
//													 * assume failure to import
//													 */
//													loop_success_flag = false;
//
//													/* date */
//													column_header_s = ConcatenateStrings (key_s, DATE_ENDING_S);
//													if (column_header_s)
//														{
//															const char *date_s = GetJSONString (observation_json_p, column_header_s);
//
//															if (date_s)
//																{
//																	observation_date_p = GetTimeFromString (date_s);
//
//																	if (observation_date_p)
//																		{
//																			if (!AddStringToStringLinkedList (processed_keys_p, column_header_s, MF_DEEP_COPY))
//																				{
//
//																				}
//																		}
//																}
//
//															FreeCopiedString (column_header_s);
//														}		/* if (column_header_s) */
//
//													if (observation_id_p)
//														{
//															observation_p = AllocateObservation (observation_id_p, observation_date_p, measured_variable_p, raw_value_s, corrected_value_s, growth_stage_s, method_s, instrument_p, nature);
//
//															if (observation_p)
//																{
//																	if (AddObservationToBaseRow (row_p, observation_p))
//																		{
//																			++ imported_obs;
//																			added_phenotype_flag = true;
//																			loop_success_flag = true;
//																		}
//																	else
//																		{
//																			char id_s [MONGO_OID_STRING_BUFFER_SIZE];
//
//																			bson_oid_to_string (row_p -> ro_id_p, id_s);
//
//																			PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, observation_json_p, "AddObservationToBaseRow failed for row \"%s\" and key \"%s\"", id_s, key_s);
//																			FreeObservation (observation_p);
//																		}
//
//																}		/* if (observation_p) */
//															else
//																{
//																	char id_s [MONGO_OID_STRING_BUFFER_SIZE];
//
//																	bson_oid_to_string (row_p -> ro_id_p, id_s);
//
//																	PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, observation_json_p, "Failed to allocate Observation for row \"%s\" and key \"%s\"", id_s, key_s);
//
//																	FreeBSONOid (observation_id_p);
//																}
//
//														}		/* if (observation_id_p) */
//													else
//														{
//															PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, observation_json_p, "Failed to allocate observation id");
//														}
//
//													if (observation_date_p)
//														{
//															FreeTime (observation_date_p);
//														}
//												}		/* if ((!IsStringEmpty (raw_value_s)) || (!IsStringEmpty (corrected_value_s))) */
//											else
//												{
//													PrintJSONToLog (STM_LEVEL_INFO, __FILE__, __LINE__, observation_json_p, "No measured value for \"%s\", skipping", key_s);
//												}
//
//											if (!added_phenotype_flag)
//												{
//													FreeMeasuredVariable (measured_variable_p);
//												}
//
//										}		/* if (phenotype_p) */
//									else
//										{
//											PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Failed to get phenotype with variable name \"%s\"", key_s);
//										}
//
//								}		/* if (! (DoesStringEndWith (mapped_key_s, "date"))) */
//
//						}		/* if ((strcmp (key_s, S_PLOT_INDEX_S) != 0) && (strcmp (key_s, S_RACK_S) != 0)) */
//
//					iterator_p = json_object_iter_next (observation_json_p, iterator_p);
//				}		/* while (iterator_p && loop_success_flag) */
//
//
//			FreeLinkedList (processed_keys_p);
//		}		/* if (processed_keys_p) */
//
//
//	if (imported_obs == total_obs)
//		{
//			status = OS_SUCCEEDED;
//		}
//	else if (imported_obs > 0)
//		{
//			status = OS_PARTIALLY_SUCCEEDED;
//		}
//
//	return status;
//}


bool AddSearchRowParams (ServiceData *data_p, ParameterSet *param_set_p)
{
	bool success_flag = false;
	Parameter *param_p = NULL;
	ParameterGroup *group_p = CreateAndAddParameterGroupToParameterSet ("Rows", false, data_p, param_set_p);

	if ((param_p = EasyCreateAndAddStringParameterToParameterSet (data_p, param_set_p, group_p, PL_ID.npt_type, PL_ID.npt_name_s, "Id", "The id of the Plot", NULL, PL_ADVANCED)) != NULL)
		{
			success_flag = true;
		}
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add %s parameter", PL_ID.npt_name_s);
		}

	return success_flag;
}



OperationStatus AddStatsValuesToBaseRow (Row *row_p, json_t *stats_json_p, Study *study_p, ServiceJob *job_p, const uint32 row_index, FieldTrialServiceData *data_p)
{
	OperationStatus status = OS_FAILED;

	return status;
}



OperationStatus AddObservationValueToStandardRow (StandardRow *row_p, const uint32 row_index, const char *key_s, const json_t *value_p, ServiceJob *job_p,  FieldTrialServiceData *data_p)
{
	OperationStatus status = OS_IDLE;


	/*
	 * ignore our column names
	 */
	if ((strcmp (key_s, S_PLOT_INDEX_S) != 0) && (strcmp (key_s, S_RACK_S) != 0))
		{
			const char *value_s = json_string_value (value_p);

			if (!IsStringEmpty (value_s))
				{
					MeasuredVariable *measured_variable_p = NULL;
					MEM_FLAG measured_variable_mem = MF_ALREADY_FREED;
					bool notes_flag = false;
					ObservationMetadata *metadata_p = NULL;

					OperationStatus obs_metadata_status = GetObservationMetadata (key_s, &measured_variable_p, &metadata_p, &notes_flag, job_p, row_index, &measured_variable_mem, data_p);

					if (obs_metadata_status == OS_SUCCEEDED)
						{
							bool free_measured_variable_flag = false;
							const json_t *raw_value_p = NULL;
							const json_t *corrected_value_p = NULL;
							ObservationError error_obj;
							char *notes_s = NULL;
							
							error_obj.oe_row = row_index;
							error_obj.oe_column_s = key_s;

							if (metadata_p -> om_corrected_flag)
								{
									corrected_value_p = value_p;
								}
							else
								{
									raw_value_p = value_p;
								}
								


							status = AddObservationValueToStandardRowByParts (job_p, row_p, measured_variable_p, metadata_p, key_s, raw_value_p,
																																corrected_value_p, notes_s, &free_measured_variable_flag, SetObservationError, &error_obj);



							if (metadata_p)
								{
									FreeObservationMetadata (metadata_p);
									metadata_p = NULL;
								}

							/*
							 * If the Observation failed to be allocated then, we need to free the
							 * Measured Variable as FreeObservation () would normally take care of
							 * that when the
							 */
							if (free_measured_variable_flag && measured_variable_p)
								{
									if (!HasMeasuredVariableCache (data_p))
										{
											FreeMeasuredVariable (measured_variable_p);
											measured_variable_p = NULL;
										}
								}

							if (notes_s)
								{
									FreeCopiedString (notes_s);
									notes_s = NULL;
								}
						}		/* if (GetObservationMetadata (key_s, &measured_variable_p, &start_date_p, &end_date_p, &corrected_value_flag, &observation_index, job_p, row_index, &measured_variable_mem, data_p)) */

				}		/* if ((!IsStringEmpty (raw_value_s)) */
			else
				{
					PrintLog (STM_LEVEL_FINER, __FILE__, __LINE__, "Skipping empty value for \"%s\" on row " UINT32_FMT, key_s, row_index);
					status = OS_SUCCEEDED;
				}


		}		/* if ((strcmp (key_s, S_PLOT_INDEX_S) != 0) && (strcmp (key_s, S_RACK_S) != 0)) */
	else
		{
			status = OS_IDLE;
		}

	return status;
}


OperationStatus AddObservationValueToStandardRowByParts (ServiceJob *job_p, StandardRow *row_p, MeasuredVariable *measured_variable_p, ObservationMetadata *metadata_p,
											const char *key_s, const json_t *raw_value_p, const json_t *corrected_value_p, const char *notes_s, bool *free_measured_variable_flag_p,
											void (*on_error_callback_fn) (ServiceJob *job_p, const char * const observation_field_s, const void *value_p, void *user_data_p), void *user_data_p)
{
	OperationStatus status = OS_FAILED;
	Observation *observation_p = NULL;
	const char *growth_stage_s = NULL;
	const char *method_s = NULL;
	ObservationNature nature = ON_ROW;
	Instrument *instrument_p = NULL;
	ObservationNode *observation_node_p = GetMatchingObservationNode (row_p, measured_variable_p, metadata_p, true);

	if (observation_node_p)
		{
			uint32 count = 0;
			uint32 num_values = 0;

			observation_p = observation_node_p -> on_observation_p;
			
			/*
			 * if both the values are json_null (), then we can remove the observation
			 */
			if ((json_is_null (raw_value_p)) && (json_is_null (corrected_value_p)))
				{
					/*
					 * We have an empty value so remove the existing observation
					 */
					RemoveObservationNode (row_p, observation_node_p);		
					status = OS_SUCCEEDED;			
				}
			else
				{
					if (raw_value_p)
						{
							++ num_values;
							
							if (SetObservationRawValueFromJSON (observation_p, raw_value_p))
								{
									++ count;
								}
							else
								{
									char id_s [MONGO_OID_STRING_BUFFER_SIZE];

									bson_oid_to_string (row_p -> sr_base.ro_id_p, id_s);

									PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, raw_value_p, "SetObservationRawValueFromJSON () failed for row \"%s\" and key \"%s\"", id_s, key_s);						

									if (on_error_callback_fn)
										{
											on_error_callback_fn (job_p, OB_RAW_VALUE_S, raw_value_p, user_data_p);
										}
								}
						}

					if (corrected_value_p)
						{
							++ num_values;
							
							if (SetObservationCorrectedValueFromJSON (observation_p, corrected_value_p))
								{
									++ count;
								}
							else
								{
									char id_s [MONGO_OID_STRING_BUFFER_SIZE];

									bson_oid_to_string (row_p -> sr_base.ro_id_p, id_s);

									PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, corrected_value_p, "SetObservationCorrectedValueFromJSON () failed for row \"%s\" and key \"%s\"", id_s, key_s);						

									if (on_error_callback_fn)
										{
											on_error_callback_fn (job_p, OB_CORRECTED_VALUE_S, corrected_value_p, user_data_p);
										}

								}
						}			

					if (notes_s)
						{
							++ num_values;

							if (IsStringEmpty (notes_s))
								{
									if (observation_p -> ob_notes_s)
										{
											FreeCopiedString (observation_p -> ob_notes_s);
											observation_p -> ob_notes_s = NULL;
										}

									++ count;
								}
							else
								{
									char *copied_notes_s = EasyCopyToNewString (notes_s);

									if (copied_notes_s)
										{
											if (observation_p -> ob_notes_s)
												{
													FreeCopiedString (observation_p -> ob_notes_s);
													observation_p -> ob_notes_s = copied_notes_s;

													++ count;
												}

										}
									else
										{
											PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to copy observation note \"%s\"", notes_s);

											if (on_error_callback_fn)
												{
													on_error_callback_fn (job_p, OB_NOTES_S, notes_s, user_data_p);
												}

										}
								}
						}

					if (count == num_values)
						{
							status = OS_SUCCEEDED;
						}
					else if (count > 0)
						{
							status = OS_PARTIALLY_SUCCEEDED;
						}
					
				}
			
			*free_measured_variable_flag_p = true;

		}		/* if (observation_node_p) */
	else
		{
			bson_oid_t *observation_id_p = GetNewBSONOid ();

			if (observation_id_p)
				{
					const ScaleClass *class_p = GetMeasuredVariableScaleClass (measured_variable_p);

					if (class_p)
						{
							ObservationType obs_type = GetObservationTypeForScaleClass (class_p);

							if (obs_type != OT_NUM_TYPES)
								{
									observation_p = AllocateObservationWithErrorHandler (observation_id_p, metadata_p, measured_variable_p, MF_SHALLOW_COPY, raw_value_p,
																																			 corrected_value_p, growth_stage_s, method_s, instrument_p, nature,
																																			 notes_s, obs_type, on_error_callback_fn, job_p, user_data_p);
								}

							if (observation_p)
								{
									if (AddObservationToStandardRow (row_p, observation_p))
										{
											status = OS_SUCCEEDED;
										}
									else
										{
											char id_s [MONGO_OID_STRING_BUFFER_SIZE];
											char *raw_value_s = NULL;
											char *corrected_value_s = NULL;
											
											if (raw_value_p)
												{
													raw_value_s = json_dumps (raw_value_p, 0);
												}

											if (corrected_value_p)
												{
													corrected_value_s = json_dumps (corrected_value_p, 0);
												}

											bson_oid_to_string (row_p -> sr_base.ro_id_p, id_s);

											PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "AddObservationToStandardRow failed for row \"%s\" and key \"%s\" with values \"%s\" and \"%s\"", id_s, key_s, 
																	raw_value_s ? raw_value_s : "NULL", corrected_value_s ? corrected_value_s : "NULL");

											if (!AddGeneralErrorMessageToServiceJob (job_p, "Failed to add Observation"))
												{
													PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add error message to service job");
												}


											if (raw_value_s)
												{
													free (raw_value_s);
												}

											if (corrected_value_s)
												{
													free (corrected_value_s);
												}

											FreeObservation (observation_p);
										}

								}		/* if (observation_p) */
							else
								{
									char id_s [MONGO_OID_STRING_BUFFER_SIZE];

									bson_oid_to_string (row_p -> sr_base.ro_id_p, id_s);

									char *raw_value_s = NULL;
									char *corrected_value_s = NULL;
									
									if (raw_value_p)
										{
											raw_value_s = json_dumps (raw_value_p, 0);
										}

									if (corrected_value_p)
										{
											corrected_value_s = json_dumps (corrected_value_p, 0);
										}

									bson_oid_to_string (row_p -> sr_base.ro_id_p, id_s);

									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate Observation for row \"%s\" and key \"%s\" with values \"%s\" and \"%s\"", id_s, key_s, 
															raw_value_s ? raw_value_s : "NULL", corrected_value_s ? corrected_value_s : "NULL");

									if (!AddGeneralErrorMessageToServiceJob (job_p, "Failed to create Observation"))
										{
											PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add error message to service job");
										}


									if (raw_value_s)
										{
											free (raw_value_s);
										}

									if (corrected_value_s)
										{
											free (corrected_value_s);
										}



									*free_measured_variable_flag_p = true;
								}

						}		/* if (class_p) */
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to get Scale Class for Measured Variable \"%s\"", GetMeasuredVariableName (measured_variable_p));
							*free_measured_variable_flag_p = true;

							if (!AddGeneralErrorMessageToServiceJob (job_p, "Failed to create Observation"))
								{
									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add error message to service job");
								}

						}

				}
			else
				{
					char *raw_value_s = NULL;
					char *corrected_value_s = NULL;
					
					if (raw_value_p)
						{
							raw_value_s = json_dumps (raw_value_p, 0);
						}

					if (corrected_value_p)
						{
							corrected_value_s = json_dumps (corrected_value_p, 0);
						}
									
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate observation id for \"%s\" with values \"%s\" and \"%s\"", key_s,
											raw_value_s ? raw_value_s : "NULL", corrected_value_s ? corrected_value_s : "NULL");

					if (raw_value_s)
						{
							free (raw_value_s);
						}

					if (corrected_value_s)
						{
							free (corrected_value_s);
						}					

					if (!AddGeneralErrorMessageToServiceJob (job_p, "Failed to create Observation"))
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add error message to service job");
						}
					
				}
		}

	return status;
}



//OperationStatus AddObservationValuesToStandardRow (StandardRow *row_p, const char *key_s, const json_t *value_p, Study *study_p, ServiceJob *job_p, const uint32 row_index, FieldTrialServiceData *data_p)
//{
//	OperationStatus status = OS_IDLE;
//
//	/*
//	 * ignore our column names
//	 */
//	if ((strcmp (key_s, S_PLOT_INDEX_S) != 0) && (strcmp (key_s, S_RACK_S) != 0))
//		{
//			MeasuredVariable *measured_variable_p = NULL;
//			MEM_FLAG mv_mem = MF_ALREADY_FREED;
//			struct tm *start_date_p = NULL;
//			struct tm *end_date_p = NULL;
//			bool corrected_value_flag = false;
//			uint32 observation_index = OB_DEFAULT_INDEX;
//
//			status = GetObservationMetadata (key_s, &measured_variable_p, &start_date_p, &end_date_p, &corrected_value_flag, &observation_index, job_p, row_index, &mv_mem, data_p);
//
//			if (status == OS_SUCCEEDED)
//				{
//					ObservationNode *observation_node_p = NULL;
//					Observation *observation_p = NULL;
//					bool free_measured_variable_flag = false;
//					const char *growth_stage_s = NULL;
//					const char *method_s = NULL;
//					ObservationNature nature = ON_ROW;
//					Instrument *instrument_p = NULL;
//					const json_t *raw_value_p = NULL;
//					const json_t *corrected_value_p = NULL;
//					bool (*set_observation_value_fn) (Observation *observation_p, const json_t *value_p) = NULL;
//
//
//					/* reset status */
//					status = OS_FAILED;
//
//					if (corrected_value_flag)
//						{
//							corrected_value_p = value_p;
//							set_observation_value_fn = SetObservationCorrectedValueFromJSON;
//						}
//					else
//						{
//							raw_value_p = value_p;
//							set_observation_value_fn = SetObservationRawValueFromJSON;
//						}
//
//					observation_node_p = GetMatchingObservationNode (row_p, measured_variable_p, start_date_p, end_date_p, &observation_index);
//
//					if (observation_node_p)
//						{
//							/*
//							 * Is the json value non-trivial?
//							 */
//							if (!IsJSONEmpty (value_p))
//								{
//
//									if (set_observation_value_fn (observation_p, value_p))
//										{
//											status = OS_SUCCEEDED;
//										}
//									else
//										{
//											char id_s [MONGO_OID_STRING_BUFFER_SIZE];
//
//											bson_oid_to_string (row_p -> sr_base.ro_id_p, id_s);
//
//											PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, value_p, "SetObservation%sValue failed for row \"%s\" and key \"%s\"", corrected_value_flag ? "Corrected" : "Raw", id_s, key_s);
//											//FreeObservation (observation_p);
//										}
//
//								}		/* if (!IsJSONEmpty (value_p)) */
//							else
//								{
//									/*
//									 * We have an empty value so remove the existing observation
//									 */
//									RemoveObservationNode (row_p, observation_node_p);
//								}
//
//							free_measured_variable_flag = true;
//						}		/* if (observation_node_p) */
//					else
//						{
//							/*
//							 * Are the json values non-trivial?
//							 */
//							if (! ((IsJSONEmpty (raw_value_p)) && (IsJSONEmpty (corrected_value_p))))
//								{
//									bson_oid_t *observation_id_p = GetNewBSONOid ();
//
//									if (observation_id_p)
//										{
//											const ScaleClass *class_p = GetMeasuredVariableScaleClass (measured_variable_p);
//											ObservationType obs_type = GetObservationTypeForScaleClass (class_p);
//
//											if (obs_type != OT_NUM_TYPES)
//												{
//													observation_p = AllocateObservation (observation_id_p, start_date_p, end_date_p, measured_variable_p, mv_mem, raw_value_p, corrected_value_p, growth_stage_s, method_s, instrument_p, nature, &observation_index, obs_type);
//												}
//											else
//												{
//													PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "GetObservationTypeForScaleClass () failed for \"%s\" and variable \"%s\"", class_p -> sc_name_s, GetMeasuredVariableName (measured_variable_p));
//												}
//
//
//											if (observation_p)
//												{
//													if (AddObservationToStandardRow (row_p, observation_p))
//														{
//															status = OS_SUCCEEDED;
//														}
//													else
//														{
//															char id_s [MONGO_OID_STRING_BUFFER_SIZE];
//
//															bson_oid_to_string (row_p -> sr_base.ro_id_p, id_s);
//
//															PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "AddObservationToStandardRow failed for row id \"%s\" and spreadsheet row " UINT32_FMT " and column \"%s\"", id_s, row_index, key_s);
//															AddTabularParameterErrorMessageToServiceJob (job_p, PL_PLOT_TABLE.npt_name_s, PL_PLOT_TABLE.npt_type, "Failed to add observed measured variable", row_index, key_s);
//
//															FreeObservation (observation_p);
//														}
//
//												}		/* if (observation_p) */
//											else
//												{
//													char id_s [MONGO_OID_STRING_BUFFER_SIZE];
//
//													bson_oid_to_string (row_p -> sr_base.ro_id_p, id_s);
//
//													PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate Observation for row id \"%s\" and spreadsheet row " UINT32_FMT " and column \"%s\"", id_s, row_index, key_s);
//													AddTabularParameterErrorMessageToServiceJob (job_p, PL_PLOT_TABLE.npt_name_s, PL_PLOT_TABLE.npt_type, "Invalid value", row_index, key_s);
//
//													free_measured_variable_flag = true;
//												}
//
//										}		/* if (observation_id_p) */
//									else
//										{
//											PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate observation id for row number " UINT32_FMT " and key \"%s\"", row_index, key_s);
//										}
//								}		/* if (! ((IsJSONEmpty (raw_value_p)) && (IsJSONEmpty (corrected_value_p)))) */
//							else
//								{
//									/*
//									 * A new Observation but with no value e.g. empty row-column entry in the
//									 * spreadsheet.
//									 */
//									status = OS_SUCCEEDED;
//								}
//						}
//
//
//
//					if (start_date_p)
//						{
//							FreeTime (start_date_p);
//							start_date_p = NULL;
//						}
//
//					if (end_date_p)
//						{
//							FreeTime (end_date_p);
//							end_date_p = NULL;
//						}
//
//					/*
//					 * If the Observation failed to be allocated then, we need to free the
//					 * Measured Variable as FreeObservation () would normally take care of
//					 * that when the
//					 */
//					if (free_measured_variable_flag && measured_variable_p)
//						{
//							if ((mv_mem == MF_SHALLOW_COPY) || (mv_mem == MF_DEEP_COPY))
//								{
//									FreeMeasuredVariable (measured_variable_p);
//									measured_variable_p = NULL;
//								}
//						}
//
//				}		/* if (GetObservationMetadata (key_s, &measured_variable_p, &start_date_p, &end_date_p, data_p)) */
//
//
//		}		/* if ((strcmp (key_s, S_PLOT_INDEX_S) != 0) && (strcmp (key_s, S_RACK_S) != 0)) */
//
//	return status;
//}



OperationStatus AddSingleTreatmentFactorValueToStandardRow  (StandardRow *row_p, const char *key_s, const char *name_s, Study *study_p, ServiceJob *job_p, const uint32 row_index, FieldTrialServiceData *data_p)
{
	OperationStatus status = OS_IDLE;
	void *temp_p = NULL;
	json_t *value_p;
	Treatment *treatment_p = NULL;
	bool cached_treatment_flag = false;

	if (data_p -> dftsd_treatments_cache_p)
		{
			treatment_p = GetCachedTreatmentByURL (data_p, name_s);
		}

	if (!treatment_p)
		{
			treatment_p = GetTreatmentByURL (key_s, VF_STORAGE, data_p);

			if (treatment_p)
				{
					if (data_p -> dftsd_treatments_cache_p)
						{
							if (AddTreatmentToCache (data_p, treatment_p, MF_SHALLOW_COPY))
								{
									cached_treatment_flag = true;
								}
							else
								{

								}
						}
				}
		}

	if (treatment_p)
		{
			/*
			 * Does the Study have a TreatmentFactor for this
			 * Treatment?
			 */
			TreatmentFactor *tf_p = GetTreatmentFactorForStudy (study_p, treatment_p -> tr_id_p, data_p);

			if (tf_p)
				{
					const char *value_s = GetTreatmentFactorValue (tf_p, name_s);

					/* Is it a valid defined label? */
					if (IsStringEmpty (value_s))
						{
							/* nothing to do */
							status = OS_SUCCEEDED;
						}
					else
						{
							if (AddTreatmentFactorValueToRowByParts (row_p, tf_p, name_s))
								{
									status = OS_SUCCEEDED;
								}
						}

					//FreeTreatmentFactor (tf_p);
				}

			if (!cached_treatment_flag)
				{
					FreeTreatment (treatment_p);
				}

		}		/* if (treatment_p) */


	return status;
}


void RemoveObservationNode (const StandardRow *row_p, ObservationNode *node_p)
{
	LinkedListRemove (row_p -> sr_observations_p, & (node_p -> on_node));
	FreeObservationNode (& (node_p -> on_node));
}


ObservationNode *GetMatchingObservationNode (const StandardRow *row_p, const MeasuredVariable *variable_p, const ObservationMetadata *metadata_p, const bool include_metadata_flag)
{
	ObservationNode *node_p = (ObservationNode *) (row_p -> sr_observations_p -> ll_head_p);

	while (node_p)
		{
			Observation *existing_observation_p = node_p -> on_observation_p;

			if (AreObservationsMatchingByParts (existing_observation_p, variable_p, metadata_p, include_metadata_flag))
				{
					return node_p;
				}
			else
				{
					node_p = (ObservationNode *) (node_p -> on_node.ln_next_p);
				}
		}

	return NULL;
}


Observation *GetMatchingObservation (const StandardRow *row_p, const MeasuredVariable *variable_p, const ObservationMetadata *metadata_p, const bool include_metadata_flag)
{
	ObservationNode *node_p = GetMatchingObservationNode (row_p, variable_p, metadata_p, include_metadata_flag);

	if (node_p)
		{
			return (node_p -> on_observation_p);
		}

	return NULL;
}


Row *GetRowByStudyIndex (const int32 by_study_index, Study *study_p, const ViewFormat format, FieldTrialServiceData *data_p)
{
	Row *row_p = NULL;
	char *index_key_s = ConcatenateVarargsStrings (PL_ROWS_S, ".", RO_STUDY_INDEX_S, NULL);

	if (index_key_s)
		{
			char *study_key_s = ConcatenateVarargsStrings (PL_ROWS_S, ".", RO_STUDY_ID_S, NULL);

			if (study_key_s)
				{
					bson_t *query_p = BCON_NEW (index_key_s, BCON_INT32 (by_study_index), study_key_s, BCON_OID (study_p -> st_id_p));

					if (query_p)
						{
							MongoTool *mongo_p = GetConfiguredMongoTool (data_p, NULL);

							if (mongo_p)
								{
									if (SetMongoToolCollection (mongo_p, data_p -> dftsd_collection_ss [DFTD_PLOT]))
										{
											json_t *results_p = GetAllMongoResultsAsJSON (mongo_p, query_p, NULL);

											if (results_p)
												{
													if (json_is_array (results_p))
														{
															const size_t num_results = json_array_size (results_p);

															if (num_results == 1)
																{
																	size_t i = 0;
																	json_t *plot_json_p = json_array_get (results_p, i);
																	Plot *plot_p = GetPlotFromJSON (plot_json_p, study_p, format, data_p);

																	if (plot_p)
																		{
																			row_p = GetRowFromPlotByStudyIndex (plot_p, by_study_index);

																			if (!row_p)
																				{
																					PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, plot_json_p, "Failed to get row with study index " UINT32_FMT,  by_study_index);
																					FreePlot (plot_p);
																				}
																		}
																	else
																		{
																			PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, plot_json_p, "Failed to create plot");
																		}

																}		/* if (num_results == 1) */
															else
																{
																	json_t *query_json_p = ConvertBSONToJSON (query_p, NULL);
																	bool printed_error_flag = false;

																	if (query_json_p)
																		{
																			char *query_json_s = json_dumps (query_json_p, 0);

																			if (query_json_s)
																				{
																					PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, results_p, UINT32_FMT " results for \"%s\"", query_json_s);
																					free (query_json_s);

																					printed_error_flag = true;
																				}

																			json_decref (query_json_p);
																		}

																	if (!printed_error_flag)
																		{
																			PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, results_p, UINT32_FMT " for row " UINT32_FMT " in study \"%s\"", by_study_index, study_p -> st_name_s);
																		}

																}

														}		/* if (json_is_array (results_p)) */

													json_decref (results_p);
												}		/* if (results_p) */
											else
												{
													PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "No results for row " UINT32_FMT " in study \"%s\"", by_study_index, study_p -> st_name_s);
												}

										}		/* if (SetMongoToolCollection (data_p -> dftsd_mongo_p, data_p -> dftsd_collection_ss [DFTD_ROW]) */


									FreeMongoTool (mongo_p);
								}		/* if (mongo_p) */
							else
								{
									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "GetConfiguredMongoTool () failed");
								}



							bson_destroy (query_p);
						}		/* if (query_p) */
					else
						{
							char *study_id_s = GetBSONOidAsString (study_p -> st_id_p);

							if (study_id_s)
								{
									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to create query for row " UINT32_FMT " in study \"%s\"", by_study_index, study_id_s);
									FreeCopiedString (study_id_s);
								}
							else
								{
									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to create query for row " UINT32_FMT " in study \"%s\"", study_p -> st_name_s);
								}
						}

					FreeCopiedString (study_key_s);
				}		/* if (study_key_s) */
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to concatenate \"%s\", \".\" and \"%s\"", PL_ROWS_S, RO_STUDY_ID_S);
				}

			FreeCopiedString (index_key_s);
		}		/* if (index_key_s) */
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to concatenate \"%s\", \".\" and \"%s\"", PL_ROWS_S, RO_STUDY_INDEX_S);
		}

	return row_p;
}


bool AddTreatmentFactorValueToRowByParts (StandardRow *row_p, TreatmentFactor *tf_p, const char *value_s)
{
	bool success_flag = false;
	TreatmentFactorValue *tf_value_p = AllocateTreatmentFactorValue (tf_p, value_s);

	if (tf_value_p)
		{
			success_flag = AddTreatmentFactorValueToStandardRow (row_p, tf_value_p);
		}
	else
		{
			const char * const tf_name_s = GetTreatmentFactorName (tf_p);

			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "AllocateTreatmentFactorValue () failed for \"%s\", and \"%s\"", tf_name_s, value_s);
		}

	return success_flag;
}



bool GetDiscardValueFromSubmissionJSON (const json_t *row_json_p)
{
	const char *value_s = GetJSONString (row_json_p, RO_DISCARD_S);

	if (value_s)
		{
			if (Stricmp (value_s, "1") == 0)
				{
					return true;
				}
		}		/* if (value_s) */

	value_s = GetJSONString (row_json_p, PL_ACCESSION_TABLE_TITLE_S);
	if (value_s)
		{
			if (Stricmp (value_s, "discard") == 0)
				{
					return true;
				}
		}		/* if (value_s) */

	return false;
}



bool GetBlankValueFromSubmissionJSON (const json_t *row_json_p)
{
	const char *value_s = GetJSONString (row_json_p, RO_BLANK_S);

	if (value_s)
		{
			if (Stricmp (value_s, "1") == 0)
				{
					return true;
				}
		}		/* if (value_s) */

	value_s = GetJSONString (row_json_p, PL_ACCESSION_TABLE_TITLE_S);
	if (value_s)
		{
			if (Stricmp (value_s, "blank") == 0)
				{
					return true;
				}
		}		/* if (value_s) */

	return false;
}


char *GetRowsNameKey (void)
{
	char *key_s = ConcatenateVarargsStrings (PL_ROWS_S, ".", MONGO_ID_S, NULL);

	if (key_s)
		{
			return key_s;
		}
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "ConcatenateVarargsStrings () failed for \"%s\", \".\" and \"%s\"", PL_ROWS_S, MONGO_ID_S);
		}

	return NULL;
}


void FreeRowsNameKey (char *key_s)
{
	FreeCopiedString (key_s);
}


Row *GetRowByIdString (const char *row_id_s, const ViewFormat format, const FieldTrialServiceData *data_p)
{
	Row *found_row_p = NULL;
	char *row_key_s = GetRowsNameKey ();

	if (row_key_s)
		{
			bson_oid_t *row_id_p = GetBSONOidFromString (row_id_s);

			if (row_id_p)
				{
					Plot *parent_plot_p = GetDFWObjectByNamedIdString (row_id_s, DFTD_PLOT, row_key_s, GetPlotCallback, format, data_p);

					if (parent_plot_p)
						{
							RowNode *row_node_p = (RowNode *) (parent_plot_p -> pl_rows_p -> ll_head_p);

							while (row_node_p)
								{
									Row *r_p = row_node_p -> rn_row_p;

									if (bson_oid_equal (row_id_p, r_p -> ro_id_p))
										{
											found_row_p = r_p;
											row_node_p = NULL;		/* force exit from loop */
										}
									else
										{
											row_node_p = (RowNode *) (row_node_p -> rn_node.ln_next_p);
										}
								}

						}

					FreeBSONOid (row_id_p);
				}


			FreeRowsNameKey (row_key_s);
		}		/* if (row_key_s) */

	return found_row_p;
}




static void SetObservationError (ServiceJob *job_p, const char * const observation_field_s, const void *value_p, void *user_data_p)
{
	const ObservationError *error_obj_p = (const ObservationError *) user_data_p;


	if (strcmp (observation_field_s, OB_RAW_VALUE_S) == 0)
		{
			AddTabularParameterErrorMessageToServiceJob (job_p, PL_PLOT_TABLE.npt_name_s, PL_PLOT_TABLE.npt_type, "Failed to set raw value for Observation", error_obj_p -> oe_row, error_obj_p -> oe_column_s);
		}
	else if (strcmp (observation_field_s, OB_CORRECTED_VALUE_S) == 0)
		{
			AddTabularParameterErrorMessageToServiceJob (job_p, PL_PLOT_TABLE.npt_name_s, PL_PLOT_TABLE.npt_type, "Failed to set corrected value for Observation", error_obj_p -> oe_row, error_obj_p -> oe_column_s);
		}
	else if (strcmp (observation_field_s, OB_NOTES_S) == 0)
		{
			AddTabularParameterErrorMessageToServiceJob (job_p, PL_PLOT_TABLE.npt_name_s, PL_PLOT_TABLE.npt_type, "Failed to set notes for Observation", error_obj_p -> oe_row, error_obj_p -> oe_column_s);
		}
	else
		{
			AddGeneralErrorMessageToServiceJob (job_p, "Failed to process Observation");
		}
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


