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
 * observation.c
 *
 *  Created on: 30 Oct 2018
 *      Author: billy
 */

#include <string.h>

#include <bson/bson.h>

#include "dfw_field_trial_service_data.h"
#include "instrument.h"
#include "jansson.h"
#include "json_util.h"
#include "linked_list.h"
#include "measured_variable.h"
#include "mongodb_tool.h"
#include "streams.h"
#include "typedefs.h"
#include "math_utils.h"
#include "mongodb_util.h"

#define ALLOCATE_OBSERVATION_TAGS (1)
#include "observation.h"

#include "time_util.h"
#include "memory_allocations.h"
#include "string_utils.h"
#include "dfw_util.h"

#include "numeric_observation.h"
#include "string_observation.h"
#include "integer_observation.h"
#include "time_observation.h"


static const char *S_OBSERVATION_NATURES_SS [ON_NUM_PHENOTYPE_NATURES] = { "Row", "Experimental Area" };

static const char *S_OBSERVATION_TYPES_SS [OT_NUM_TYPES] = { "xsd:double", "xsd:string", 	"params:signed_integer",	"xsd:date"};


static bool AddObservationNatureToJSON (const ObservationNature phenotype_nature, json_t *doc_p);

static bool GetObservationNatureFromJSON (ObservationNature *phenotype_nature_p, const json_t *doc_p);

static bool CreateInstrumentFromObservationJSON (const json_t *observation_json_p, Instrument **instrument_pp, const FieldTrialServiceData *data_p);

static MeasuredVariable *CreateMeasuredVariableFromObservationJSON (const json_t *observation_json_p, MEM_FLAG *phenotype_mem_p, FieldTrialServiceData *data_p);

static bool CompareObservationDates (const struct tm * const time_0_p, const struct tm * const time_1_p);

static bool GetObservationTypeFromJSON (ObservationType *type_p, const json_t *doc_p);





/*
 * API definitions
 */


bool InitObservation (Observation *observation_p, bson_oid_t *id_p, ObservationMetadata *metadata_p, MeasuredVariable *phenotype_p, MEM_FLAG phenotype_mem,
											const char *growth_stage_s, const char *method_s, Instrument *instrument_p, const ObservationNature nature, const char *notes_s, const ObservationType obs_type,
											void (*clear_fn) (Observation *observation_p),
											bool (*add_values_to_json_fn) (const struct Observation *obs_p, const char *raw_key_s, const char *corrected_key_s, json_t *json_p, const char *null_sequence_s, bool only_if_exists_flag),
											bool (*set_value_from_json_fn) (struct Observation *observation_p, ObservationValueType ovt, const json_t *value_p),
											bool (*set_value_from_string_fn) (struct Observation *observation_p, ObservationValueType ovt, const char *value_s),
											bool (*get_value_as_string_fn) (const struct Observation *observation_p, ObservationValueType ovt, char **value_ss, bool *free_value_flag_p)
)
{
	char *copied_growth_stage_s = NULL;

	if ((IsStringEmpty (growth_stage_s)) || ((copied_growth_stage_s = EasyCopyToNewString (growth_stage_s)) != NULL))
		{
			char *copied_method_s = NULL;

			if ((IsStringEmpty (method_s)) || ((copied_method_s = EasyCopyToNewString (method_s)) != NULL))
				{
					char *copied_notes_s = NULL;

					if ((IsStringEmpty (notes_s)) || ((copied_notes_s = EasyCopyToNewString (notes_s)) != NULL))
						{
							ObservationMetadata *copied_metadata_p = NULL;

							if ((!metadata_p) || ((copied_metadata_p = CopyObservationMetadata (metadata_p)) != NULL))
								{
									observation_p -> ob_id_p = id_p;
									observation_p -> ob_phenotype_p = phenotype_p;
									observation_p -> ob_phenotype_mem = phenotype_mem;
									observation_p -> ob_metadata_p = copied_metadata_p;
									observation_p -> ob_instrument_p = instrument_p;
									observation_p -> ob_growth_stage_s = copied_growth_stage_s;
									observation_p -> ob_method_s = copied_method_s;
									observation_p -> ob_notes_s = copied_notes_s;
									observation_p -> ob_nature = nature;
									observation_p -> ob_type = obs_type;
									observation_p -> ob_clear_fn = clear_fn;
									observation_p -> ob_add_values_to_json_fn = add_values_to_json_fn;
									observation_p -> ob_set_value_from_json_fn = set_value_from_json_fn;
									observation_p -> ob_set_value_from_string_fn = set_value_from_string_fn;
									observation_p -> ob_get_value_as_string_fn = get_value_as_string_fn;

									return true;
								}
							else
								{
									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "CopyObservationMetadata () failed");
								}
						}		/* if ((IsStringEmpty (notes_s)) || ((copied_method_s = EasyCopyToNewString (notes_s)) != NULL)) */
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to copy notes_s \"%s\"", notes_s);
						}


				}		/* if ((IsStringEmpty (method_s)) || ((copied_method_s = EasyCopyToNewString (method_s)) != NULL)) */
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to copy method_s \"%s\"", method_s);
				}

			if (copied_growth_stage_s)
				{
					FreeCopiedString (copied_growth_stage_s);
				}

		}		/* if ((IsStringEmpty (growth_stage_s)) || ((copied_growth_stage_s = EasyCopyToNewString (growth_stage_s)) != NULL)) */
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to copy growth_stage_s \"%s\"", growth_stage_s);
		}

	return false;
}





Observation *AllocateObservation (bson_oid_t *id_p, ObservationMetadata *metadata_p, MeasuredVariable *phenotype_p,
																	MEM_FLAG phenotype_mem, const json_t *raw_value_p, const json_t *corrected_value_p,
																	const char *growth_stage_s, const char *method_s, Instrument *instrument_p, const ObservationNature nature,
																	const char *notes_s, const ObservationType obs_type)
{
	return AllocateObservationWithErrorHandler (id_p, metadata_p, phenotype_p, phenotype_mem, raw_value_p, corrected_value_p, growth_stage_s,
																							method_s, instrument_p, nature, notes_s, obs_type, NULL, NULL, NULL);
}

Observation *AllocateObservationWithErrorHandler (bson_oid_t *id_p, ObservationMetadata *metadata_p, MeasuredVariable *phenotype_p,
																									MEM_FLAG phenotype_mem, const json_t *raw_value_p, const json_t *corrected_value_p,
																									const char *growth_stage_s, const char *method_s, Instrument *instrument_p, const ObservationNature nature,
																									const char *notes_s, const ObservationType obs_type,
																									void (*on_error_callback_fn) (ServiceJob *job_p, const char * const observation_field_s, const void *value_p, void *user_data_p),
																									ServiceJob *job_p, void *user_data_p)
{
	Observation *observation_p = NULL;
	const ScaleClass *class_p = GetMeasuredVariableScaleClass (phenotype_p);

	if (class_p)
		{
			switch (class_p -> sc_type)
			{
				case PT_SIGNED_REAL:
					{
						double64 *raw_p = NULL;
						double64 *corrected_p = NULL;
						double64 raw_value = 0.0;
						double64 corrected_value = 0.0;

						if (raw_value_p)
							{
								if (json_is_number (raw_value_p))
									{
										raw_value = json_number_value (raw_value_p);
										raw_p = &raw_value;
									}
								else if (json_is_string (raw_value_p))
									{
										if (GetRealValueFromJSONString (raw_value_p, &raw_value))
											{
												raw_p = &raw_value;
											}
										else
											{
												const char *value_s = json_string_value (raw_value_p);

												PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "GetRealValueFromJSONString () failed for \"%s\"", value_s);

												if (on_error_callback_fn && job_p)
													{
														on_error_callback_fn (job_p, OB_RAW_VALUE_S, raw_value_p, user_data_p);
													}
											}
									}
								else
									{
										PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Invalid json Numeric type: %d", json_typeof (raw_value_p));

										if (on_error_callback_fn && job_p)
											{
												on_error_callback_fn (job_p, OB_RAW_VALUE_S, raw_value_p, user_data_p);
											}
									}
							}

						if (corrected_value_p)
							{
								if (json_is_number (corrected_value_p))
									{
										corrected_value = json_number_value (corrected_value_p);
										corrected_p = &corrected_value;
									}
								else if (json_is_string (corrected_value_p))
									{
										if (GetRealValueFromJSONString (corrected_value_p, &corrected_value))
											{
												corrected_p = &corrected_value;
											}
										else
											{
												PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "GetRealValueFromJSONString () failed for \"%s\"", json_string_value (corrected_value_p));

												if (on_error_callback_fn && job_p)
													{
														on_error_callback_fn (job_p, OB_CORRECTED_VALUE_S, corrected_value_p, user_data_p);
													}
											}
									}
								else
									{
										PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Invalid json type: %d", json_typeof (corrected_value_p));

										if (on_error_callback_fn && job_p)
											{
												on_error_callback_fn (job_p, OB_CORRECTED_VALUE_S, corrected_value_p, user_data_p);
											}
									}

							}

						if (raw_p || corrected_p)
							{
								NumericObservation *numeric_obs_p = AllocateNumericObservation (id_p, metadata_p, phenotype_p, phenotype_mem, raw_p, corrected_p, growth_stage_s, method_s, instrument_p, nature, notes_s);

								if (numeric_obs_p)
									{
										observation_p = & (numeric_obs_p -> no_base_observation);
									}
							}
					}
					break;

				case PT_UNSIGNED_INT:
				case PT_SIGNED_INT:
					{
						int32 *raw_p = NULL;
						int32 *corrected_p = NULL;
						int32 raw_value = 0;
						int32 corrected_value = 0;

						if (raw_value_p)
							{
								if (json_is_integer (raw_value_p))
									{
										raw_value = json_integer_value (raw_value_p);
										raw_p = &raw_value;
									}
								else if (json_is_string (raw_value_p))
									{
										const char *value_s = json_string_value (raw_value_p);

										if (GetValidInteger (&value_s, &raw_value))
											{
												raw_p = &raw_value;
											}
										else
											{
												PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "GetValidInteger () failed for \"%s\"", value_s);

												if (on_error_callback_fn && job_p)
													{
														on_error_callback_fn (job_p, OB_RAW_VALUE_S, raw_value_p, user_data_p);
													}
											}

									}
								else
									{
										PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Invalid json type: %d", json_typeof (raw_value_p));

										if (on_error_callback_fn && job_p)
											{
												on_error_callback_fn (job_p, OB_RAW_VALUE_S, raw_value_p, user_data_p);
											}
									}
							}

						if (corrected_value_p)
							{
								if (json_is_integer (corrected_value_p))
									{
										corrected_value = json_integer_value (corrected_value_p);
										corrected_p = &corrected_value;
									}
								else if (json_is_string (corrected_value_p))
									{
										const char *value_s = json_string_value (corrected_value_p);

										if (GetValidInteger (&value_s, &corrected_value))
											{
												corrected_p = &corrected_value;
											}
										else
											{
												PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "GetValidInteger () failed for \"%s\"", value_s);

												if (on_error_callback_fn && job_p)
													{
														on_error_callback_fn (job_p, OB_CORRECTED_VALUE_S, corrected_value_p, user_data_p);
													}
											}
									}
								else
									{
										PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Invalid json type: %d", json_typeof (corrected_value_p));

										if (on_error_callback_fn && job_p)
											{
												on_error_callback_fn (job_p, OB_CORRECTED_VALUE_S, corrected_value_p, user_data_p);
											}
									}

							}

						if (raw_p || corrected_p)
							{
								IntegerObservation *int_obs_p = AllocateIntegerObservation (id_p, metadata_p, phenotype_p, phenotype_mem, raw_p, corrected_p, growth_stage_s, method_s, instrument_p, nature, notes_s);

								if (int_obs_p)
									{
										observation_p = & (int_obs_p -> io_base_observation);
									}
							}
					}
					break;


				case PT_STRING:
					{
						const char *raw_value_s = NULL;
						const char *corrected_value_s = NULL;

						if (json_is_string (raw_value_p))
							{
								raw_value_s = json_string_value (raw_value_p);
							}

						if (json_is_string (corrected_value_p))
							{
								corrected_value_s = json_string_value (corrected_value_p);
							}

						if (raw_value_s || corrected_value_s)
							{
								StringObservation *string_obs_p = AllocateStringObservation (id_p, metadata_p, phenotype_p, phenotype_mem, raw_value_s, corrected_value_s, growth_stage_s, method_s, instrument_p, nature, notes_s);

								if (string_obs_p)
									{
										observation_p = & (string_obs_p -> so_base_observation);
									}
							}
					}
					break;

				case PT_TIME:
					{
						struct tm *raw_time_p = NULL;
						struct tm *corrected_time_p = NULL;
						bool success_flag = true;

						if (json_is_string (raw_value_p))
							{
								const char *raw_value_s = json_string_value (raw_value_p);
								raw_time_p = GetTimeFromString (raw_value_s);

								if (!raw_time_p)
									{
										success_flag = false;
										PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "GetTimeFromString () failed for \"%s\"", raw_value_s);

										if (on_error_callback_fn && job_p)
											{
												on_error_callback_fn (job_p, OB_RAW_VALUE_S, raw_value_p, user_data_p);
											}
									}
							}

						if (success_flag)
							{
								if (json_is_string (corrected_value_p))
									{
										const char *corrected_value_s = json_string_value (corrected_value_p);
										corrected_time_p = GetTimeFromString (corrected_value_s);

										if (!corrected_time_p)
											{
												success_flag = false;
												PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "GetTimeFromString () failed for \"%s\"", corrected_value_s);

												if (on_error_callback_fn && job_p)
													{
														on_error_callback_fn (job_p, OB_CORRECTED_VALUE_S, corrected_value_p, user_data_p);
													}
											}
									}
							}

						if (success_flag)
							{
								if (raw_value_p || corrected_value_p)
									{
										TimeObservation *time_obs_p = AllocateTimeObservation (id_p, metadata_p, phenotype_p, phenotype_mem, raw_time_p, corrected_time_p, growth_stage_s, method_s, instrument_p, nature, notes_s);

										if (time_obs_p)
											{
												observation_p = & (time_obs_p -> to_base_observation);
											}
									}
							}

						if (raw_time_p)
							{
								FreeTime (raw_time_p);
							}

						if (corrected_time_p)
							{
								FreeTime (corrected_time_p);
							}

					}
					break;

				default:
					break;
			}

		}
	else
		{

		}

	return observation_p;
}





void ClearObservation (Observation *observation_p)
{
	if (observation_p -> ob_id_p)
		{
			FreeBSONOid (observation_p -> ob_id_p);
			observation_p -> ob_id_p = NULL;
		}

	if (observation_p -> ob_phenotype_p)
		{
			if ((observation_p -> ob_phenotype_mem == MF_DEEP_COPY) || (observation_p -> ob_phenotype_mem == MF_SHALLOW_COPY))
				{
					FreeMeasuredVariable (observation_p -> ob_phenotype_p);
				}

			observation_p -> ob_phenotype_p = NULL;
		}

	if (observation_p -> ob_metadata_p)
		{
			ClearObservationMetadata (observation_p -> ob_metadata_p);
		}



	if (observation_p -> ob_method_s)
		{
			FreeCopiedString (observation_p -> ob_method_s);
			observation_p -> ob_method_s = NULL;
		}

	if (observation_p -> ob_notes_s)
		{
			FreeCopiedString (observation_p -> ob_notes_s);
			observation_p -> ob_notes_s = NULL;
		}


	if (observation_p -> ob_clear_fn)
		{
			observation_p -> ob_clear_fn (observation_p);
		}

}



void FreeObservation (Observation *observation_p)
{
	ClearObservation (observation_p);

	/*
	 * Still need to deallocate the instrument
	 *
	 * observation_p -> ob_instrument_p
	 */

	FreeMemory (observation_p);
}


ObservationNode *AllocateObservationNode (Observation *observation_p)
{
	ObservationNode *ob_node_p = (ObservationNode *) AllocMemory (sizeof (ObservationNode));

	if (ob_node_p)
		{
			InitListItem (& (ob_node_p -> on_node));

			ob_node_p -> on_observation_p = observation_p;
		}

	return ob_node_p;
}


void FreeObservationNode (ListItem *node_p)
{
	ObservationNode *ob_node_p = (ObservationNode *) node_p;

	if (ob_node_p -> on_observation_p)
		{
			FreeObservation (ob_node_p -> on_observation_p);
		}

	FreeMemory (ob_node_p);
}




json_t *GetObservationAsJSON (const Observation *observation_p, const ViewFormat format, const FieldTrialServiceData *data_p)
{
	json_t *observation_json_p = json_object ();

	if (observation_json_p)
		{
			if (AddObservationMetadataToJSON (observation_p -> ob_metadata_p, observation_json_p))
				{
					if ((IsStringEmpty (observation_p -> ob_growth_stage_s)) || (SetJSONString (observation_json_p, OB_GROWTH_STAGE_S, observation_p -> ob_growth_stage_s)))
						{
							if ((IsStringEmpty (observation_p -> ob_method_s)) || (SetJSONString (observation_json_p, OB_METHOD_S, observation_p -> ob_method_s)))
								{
									if ((IsStringEmpty (observation_p -> ob_notes_s)) || (SetJSONString (observation_json_p, OB_NOTES_S, observation_p -> ob_notes_s)))
										{
											if (SetJSONInteger (observation_json_p, OB_INDEX_S, observation_p -> ob_index))
												{
													bool done_objects_flag = false;

													if (format == VF_CLIENT_FULL)
														{
															bool done_instrument_flag = false;

															if (observation_p -> ob_instrument_p)
																{
																	json_t *instrument_json_p = GetInstrumentAsJSON (observation_p -> ob_instrument_p);

																	if (instrument_json_p)
																		{
																			if (json_object_set_new (observation_json_p, OB_INSTRUMENT_S, instrument_json_p) == 0)
																				{
																					done_instrument_flag = true;
																				}		/* if (json_object_set_new (observation_json_p, OB_INSTRUMENT_S, instrument_json_p) == 0) */
																			else
																				{
																					json_decref (instrument_json_p);
																				}
																		}
																}
															else
																{
																	done_instrument_flag = true;
																}

															if (done_instrument_flag)
																{
																	json_t *phenotype_json_p = GetMeasuredVariableAsJSON (observation_p -> ob_phenotype_p, format, data_p);

																	if (phenotype_json_p)
																		{
																			if (json_object_set_new (observation_json_p, OB_PHENOTYPE_S, phenotype_json_p) == 0)
																				{
																					done_objects_flag = true;
																				}		/* if (json_object_set_new (observation_json_p, OB_PHENOTYPE_S, phenotype_json_p) == 0) */
																			else
																				{
																					json_decref (phenotype_json_p);
																				}
																		}
																}

														}		/* iif (format == VF_CLIENT_FULL) */
													else if (format == VF_STORAGE)
														{
															if (AddCompoundIdToJSON (observation_json_p, observation_p -> ob_id_p))
																{
																	if ((! (observation_p -> ob_instrument_p)) || (AddNamedCompoundIdToJSON (observation_json_p, observation_p -> ob_instrument_p -> in_id_p, OB_INSTRUMENT_ID_S)))
																		{
																			if ((! (observation_p -> ob_phenotype_p)) || (AddNamedCompoundIdToJSON (observation_json_p, observation_p -> ob_phenotype_p -> mv_id_p, OB_PHENOTYPE_ID_S)))
																				{
																					if (AddObservationNatureToJSON (observation_p -> ob_nature, observation_json_p))
																						{
																							done_objects_flag = true;
																						}
																					else
																						{
																							PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, observation_json_p, "Failed to add \"%s\": %d to JSON", OB_NATURE_S, observation_p -> ob_nature);
																						}
																				}
																		}

																}		/* if (AddCompoundIdToJSON (observation_json_p, observation_p -> ob_id_p)) */
															else
																{
																	char id_s [MONGO_OID_STRING_BUFFER_SIZE];

																	bson_oid_to_string (observation_p -> ob_id_p, id_s);
																	PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, observation_json_p, "Failed to set \"%s\": \"%s\" to JSON", MONGO_ID_S, id_s);
																}

														}		/* else if (format == VF_STORAGE) */
													else if (format == VF_CLIENT_MINIMAL)
														{
															json_t *phenotype_json_p = GetMeasuredVariableAsJSON (observation_p -> ob_phenotype_p, VF_CLIENT_MINIMAL, data_p);

															if (phenotype_json_p)
																{
																	if (json_object_set_new (observation_json_p, OB_PHENOTYPE_S, phenotype_json_p) == 0)
																		{
																			done_objects_flag = true;
																		}		/* if (json_object_set_new (observation_json_p, OB_PHENOTYPE_S, phenotype_json_p) == 0) */
																	else
																		{
																			json_decref (phenotype_json_p);
																		}
																}

														}		/* else if (format == VF_CLIENT_MINIMAL) */


													if (done_objects_flag)
														{
															if (AddDatatype (observation_json_p, DFTD_OBSERVATION))
																{
																	if (observation_p -> ob_add_values_to_json_fn (observation_p, OB_RAW_VALUE_S, OB_CORRECTED_VALUE_S, observation_json_p, NULL, true))
																		{
																			return observation_json_p;
																		}
																	else
																		{
																			PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, observation_json_p, "Unknown observation type %d", observation_p -> ob_type);
																		}

																}

														}


												}		/* if ((observation_p -> ob_index == 1) || (SetJSONInteger (observation_json_p, OB_INDEX_S, observation_p -> ob_index))) */
											else
												{
													PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, observation_json_p, "Failed to set \"%s\": " UINT32_FMT " to JSON", OB_INDEX_S, observation_p -> ob_index);
												}

										}		/* if ((IsStringEmpty (observation_p -> ob_notes_s)) || (SetJSONString (observation_json_p, OB_NOTES_S, observation_p -> ob_notes_s))) */
									else
										{
											PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, observation_json_p, "Failed to set \"%s\": \"%s\" to JSON", OB_NOTES_S, observation_p -> ob_notes_s);
										}



								}		/* if ((IsStringEmpty (observation_p -> ob_method_s)) || SetJSONString (observation_json_p, OB_METHOD_S, observation_p -> ob_method_s)) */
							else
								{
									PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, observation_json_p, "Failed to set \"%s\": \"%s\" to JSON", OB_METHOD_S, observation_p -> ob_method_s);
								}

						}		/* if ((IsStringEmpty (observation_p -> ob_growth_stage_s)) || SetJSONString (observation_json_p, OB_GROWTH_STAGE_S, observation_p -> ob_growth_stage_s)) */
					else
						{
							PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, observation_json_p, "Failed to set \"%s\": \"%s\" to JSON", OB_GROWTH_STAGE_S, observation_p -> ob_growth_stage_s);
						}

				}		/* if (AddValidDateToJSON (observation_p -> ob_end_date_p, observation_json_p, OB_END_DATE_S, false)) */


			json_decref (observation_json_p);
		}		/* if (observation_json_p) */

	return NULL;
}




bool PopulateObservationFromJSON (Observation *observation_p, const json_t *observation_json_p, FieldTrialServiceData *data_p)
{
	bool success_flag = false;




	return success_flag;
}




Observation *GetObservationFromJSON (const json_t *observation_json_p, FieldTrialServiceData *data_p)
{
	Observation *observation_p = NULL;
	struct tm *start_date_p = NULL;

	if (CreateValidDateFromJSON (observation_json_p, OB_START_DATE_S, &start_date_p))
		{
			struct tm *end_date_p = NULL;

			if (CreateValidDateFromJSON (observation_json_p, OB_END_DATE_S, &end_date_p))
				{
					bson_oid_t *id_p = GetNewUnitialisedBSONOid ();

					if (id_p)
						{
							if (GetMongoIdFromJSON (observation_json_p, id_p))
								{
									Instrument *instrument_p = NULL;

									if (CreateInstrumentFromObservationJSON (observation_json_p, &instrument_p, data_p))
										{
											MEM_FLAG phenotype_mem = MF_SHALLOW_COPY;
											MeasuredVariable *phenotype_p = CreateMeasuredVariableFromObservationJSON (observation_json_p, &phenotype_mem, data_p);

											if (phenotype_p)
												{
													ObservationNature nature = ON_ROW;
													const char *growth_stage_s = GetJSONString (observation_json_p, OB_GROWTH_STAGE_S);
													const char *method_s = GetJSONString (observation_json_p, OB_METHOD_S);
													const char *notes_s = GetJSONString (observation_json_p, OB_NOTES_S);
													//const char *raw_value_s = GetJSONString (observation_json_p, OB_RAW_VALUE_S);
													//const char *corrected_value_s = GetJSONString (observation_json_p, OB_CORRECTED_VALUE_S);
													uint32 index = OB_DEFAULT_INDEX;


													const json_t *raw_value_p = json_object_get (observation_json_p, OB_RAW_VALUE_S);
													const json_t *corrected_value_p = json_object_get (observation_json_p, OB_CORRECTED_VALUE_S);

													GetJSONUnsignedInteger (observation_json_p, OB_INDEX_S,  &index);

													/*
													 * do we have a valid measurement?
													 */
													if (raw_value_p || corrected_value_p)
														{
															const ScaleClass *class_p = GetMeasuredVariableScaleClass (phenotype_p);

															if (class_p)
																{
																	ObservationType obs_type = GetObservationTypeForScaleClass (class_p);

																	if (obs_type != OT_NUM_TYPES)
																		{
																			ObservationMetadata *metadata_p = AllocateObservationMetadata (start_date_p, end_date_p, false, index);

																			if (metadata_p)
																				{
																					char *error_s = NULL;

																					GetObservationNatureFromJSON (&nature, observation_json_p);

																					observation_p = AllocateObservation (id_p, metadata_p, phenotype_p, phenotype_mem, raw_value_p, corrected_value_p, growth_stage_s, method_s,
																																							 instrument_p, nature, notes_s, class_p -> sc_type);

																					if (!observation_p)
																						{
																							PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, observation_json_p, "Failed to allocate Observation");

																							FreeObservationMetadata (metadata_p);
																						}

																				}
																		}
																}


														}		/* if (! ((IsStringEmpty (raw_value_s)) && (IsStringEmpty (corrected_value_s)))) */
													else
														{
															PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, observation_json_p, "JSON doesn't have either of \"%s\" or \"%s\"", OB_RAW_VALUE_S, OB_CORRECTED_VALUE_S);
														}

												}		/* if (phenotype_p) */

										}		/* if (CreateInstrumentFromObservationJSON (observation_json_p, &instrument_p, data_p)) */
									else
										{
											PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, observation_json_p, "Failed to create instrument");
										}

								}		/* if (GetMongoIdFromJSON (observation_json_p, id_p)) */
							else
								{
									PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, observation_json_p, "Failed to get id \"%s\"", MONGO_ID_S);
								}

							if (!observation_p)
								{
									FreeBSONOid (id_p);
								}
						}		/* if (id_p) */
					else
						{
							PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, observation_json_p, "Failed to allocate id");
						}

					if (end_date_p)
						{
							FreeTime (end_date_p);
						}

				}		/* if (CreateValidDateFromJSON (observation_json_p, OB_END_DATE_S, &end_date_p)) */

			if (start_date_p)
				{
					FreeTime (start_date_p);
				}

		}		/* if (CreateValidDateFromJSON (observation_json_p, OB_DATE_S, &start_date_p)) */
	else
		{
			PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, observation_json_p, "Failed to create date from \"%s\"", OB_START_DATE_S);
		}

	return observation_p;
}


bool SaveObservation (Observation *observation_p, const FieldTrialServiceData *data_p)
{
	bson_t *selector_p = NULL;
	bool success_flag = PrepareSaveData (& (observation_p -> ob_id_p), &selector_p);

	if (success_flag)
		{
			json_t *observation_json_p = GetObservationAsJSON (observation_p, false, data_p);

			if (observation_json_p)
				{
					MongoTool *mongo_p = GetConfiguredMongoTool (data_p, NULL);

					if (mongo_p)
						{
							success_flag = SaveAndBackupMongoDataWithTimestamp (mongo_p, observation_json_p, data_p -> dftsd_collection_ss [DFTD_OBSERVATION],
																																	data_p -> dftsd_backup_collection_ss [DFTD_OBSERVATION], DFT_BACKUPS_ID_KEY_S, selector_p, MONGO_TIMESTAMP_S);


							FreeMongoTool (mongo_p);
						}		/* if (mongo_p) */
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "GetConfiguredMongoTool () failed");
						}



					json_decref (observation_json_p);
				}		/* if (observation_json_p) */

		}		/* if (observation_p -> ob_id_p) */

	return success_flag;
}


bool AreObservationsMatching (const Observation *observation_0_p, const Observation *observation_1_p)
{
	return AreObservationsMatchingByParts (observation_0_p, observation_1_p -> ob_phenotype_p, observation_1_p -> ob_metadata_p);
}


bool AreObservationsMatchingByParts (const Observation *observation_p, const MeasuredVariable *variable_p, const ObservationMetadata *metadata_p)
{
	bool match_flag = false;

	if (bson_oid_equal (variable_p -> mv_id_p, observation_p -> ob_phenotype_p -> mv_id_p))
		{
			int res = 0;

			if (metadata_p)
				{
					res = CompareObservationMetadata (observation_p -> ob_metadata_p, metadata_p);

					if (res == 0)
						{
							if (observation_p -> ob_index == metadata_p -> om_index)
								{
									match_flag = true;
								}
						}

				}


		}

	return match_flag;
}



bool AddObservationValuesToFrictionlessData (Observation *obs_p, json_t *fd_json_p)
{
	bool success_flag = false;
	const char *variable_s = GetMeasuredVariableName (obs_p -> ob_phenotype_p);

	if (variable_s)
		{
			char *key_s = NULL;
			const ObservationMetadata *metadata_p = obs_p -> ob_metadata_p;

			if (metadata_p -> om_start_date_p)
				{
					char *start_time_s = GetTimeAsString (metadata_p -> om_start_date_p, true, NULL);

					if (start_time_s)
						{
							if (metadata_p -> om_end_date_p)
								{
									char *end_time_s = GetTimeAsString (metadata_p -> om_end_date_p, true, NULL);

									if (end_time_s)
										{
											key_s = ConcatenateVarargsStrings (variable_s, " ", start_time_s, " ", end_time_s, NULL);
											FreeCopiedString (end_time_s);
										}
								}
							else
								{
									key_s = ConcatenateVarargsStrings (variable_s, " ", start_time_s, NULL);
								}

							FreeCopiedString (start_time_s);
						}
				}
			else
				{
					key_s = (char *) variable_s;
				}

			if (key_s)
				{
					success_flag = obs_p -> ob_add_values_to_json_fn (obs_p, "raw_value", "corrected_value", fd_json_p, "-", true);

					if (key_s != variable_s)
						{
							FreeCopiedString (key_s);
						}

				}		/* if (key_s) */

		}		/* if (variable_s) */

	return success_flag;
}


bool SetObservationRawValueFromJSON (Observation *observation_p, const json_t *value_p)
{
	return observation_p -> ob_set_value_from_json_fn (observation_p, OVT_RAW_VALUE, value_p);
}



bool SetObservationRawValueFromString (Observation *observation_p, const char * const value_s)
{
	return observation_p -> ob_set_value_from_string_fn (observation_p, OVT_RAW_VALUE, value_s);
}


bool SetObservationCorrectedValueFromJSON (Observation *observation_p, const json_t *value_p)
{
	return observation_p -> ob_set_value_from_json_fn (observation_p, OVT_CORRECTED_VALUE, value_p);
}


bool SetObservationCorrectedValueFromString (Observation *observation_p, const char * const value_s)
{
	return observation_p -> ob_set_value_from_string_fn (observation_p, OVT_CORRECTED_VALUE, value_s);
}


ObservationType GetObservationTypeForScaleClass (const ScaleClass *class_p)
{
	ObservationType obs_type = OT_NUM_TYPES;

	switch (class_p -> sc_type)
	{
		case PT_SIGNED_REAL:
			obs_type = OT_NUMERIC;
			break;

		case PT_STRING:
			obs_type = OT_STRING;
			break;

		case PT_SIGNED_INT:
		case PT_UNSIGNED_INT:
			obs_type = OT_INTEGER;
			break;

		case PT_TIME:
			obs_type = OT_TIME;
			break;

		default:
			break;
	}

	return obs_type;
}


/*
 * static definitions
 */

static bool CompareObservationDates (const struct tm * const time_0_p, const struct tm * const time_1_p)
{
	bool match_flag = false;

	if (time_0_p)
		{
			if (time_1_p)
				{
					if (CompareDates (time_0_p, time_1_p, false) == 0)
						{
							match_flag = true;
						}
				}
		}
	else
		{
			if (!time_1_p)
				{
					match_flag = true;
				}
		}

	return match_flag;
}


static bool CreateInstrumentFromObservationJSON (const json_t *observation_json_p, Instrument **instrument_pp, const FieldTrialServiceData *data_p)
{
	bool success_flag = false;
	Instrument *instrument_p = NULL;
	const json_t *val_p = json_object_get (observation_json_p, OB_INSTRUMENT_S);

	if (val_p)
		{
			instrument_p = GetInstrumentFromJSON (val_p);

			if (instrument_p)
				{
					*instrument_pp = instrument_p;
					success_flag = true;
				}
		}
	else
		{
			bson_oid_t *instrument_id_p = GetNewUnitialisedBSONOid ();

			if (instrument_id_p)
				{
					if (GetNamedIdFromJSON (observation_json_p, OB_INSTRUMENT_ID_S, instrument_id_p))
						{
							instrument_p = GetInstrumentById (instrument_id_p, data_p);

							if (instrument_p)
								{
									*instrument_pp = instrument_p;
									success_flag = true;
								}

						}
					else
						{
							/* no instrument in json */
							success_flag = true;
						}

					FreeBSONOid (instrument_id_p);
				}
		}


	return success_flag;
}


static bool AddObservationNatureToJSON (const ObservationNature nature, json_t *doc_p)
{
	bool success_flag = false;

	if (nature < ON_NUM_PHENOTYPE_NATURES)
		{
			success_flag = SetJSONString (doc_p, OB_NATURE_S, * (S_OBSERVATION_NATURES_SS + nature));
		}

	return success_flag;
}


static bool GetObservationNatureFromJSON (ObservationNature *nature_p, const json_t *doc_p)
{
	bool success_flag = false;
	const char *value_s = GetJSONString (doc_p, OB_NATURE_S);

	if (value_s)
		{
			ObservationNature i = ON_ROW;

			while (i < ON_NUM_PHENOTYPE_NATURES)
				{
					if (strcmp (value_s, * (S_OBSERVATION_NATURES_SS + i)) == 0)
						{
							*nature_p = i;
							return true;
						}
					else
						{
							++ i;
						}

				}
		}

	return success_flag;
}


static bool GetObservationTypeFromJSON (ObservationType *type_p, const json_t *doc_p)
{
	const char *value_s = GetJSONString (doc_p, OB_TYPE_S);

	if (value_s)
		{
			ObservationType i = 0;

			while (i < OT_NUM_TYPES)
				{
					if (strcmp (value_s, * (S_OBSERVATION_TYPES_SS + i)) == 0)
						{
							*type_p = i;
							return true;
						}
					else
						{
							++ i;
						}

				}
		}

	return false;
}


static MeasuredVariable *CreateMeasuredVariableFromObservationJSON (const json_t *observation_json_p, MEM_FLAG *phenotype_mem_p, FieldTrialServiceData *data_p)
{
	MeasuredVariable *phenotype_p = NULL;

	/* Try getting the embedded phenotype */
	const json_t *val_p = json_object_get (observation_json_p, OB_PHENOTYPE_S);

	if (val_p)
		{
			phenotype_p = GetMeasuredVariableFromJSON (val_p, data_p);

			if (phenotype_p)
				{
					*phenotype_mem_p = MF_SHADOW_USE;
				}
			else
				{
					PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, observation_json_p, "Failed to get phenotype from json");
				}
		}
	else
		{
			/* There was not an embedded phenotype, so check for a phenotype id */
			val_p = json_object_get (observation_json_p, OB_PHENOTYPE_ID_S);

			if (val_p)
				{
					const char *oid_s = GetJSONString (val_p, MONGO_OID_KEY_S);

					if (oid_s)
						{
							const bool has_cache_flag = HasMeasuredVariableCache (data_p);

							if (has_cache_flag)
								{
									phenotype_p = GetCachedMeasuredVariableById (data_p, oid_s);

									if (phenotype_p)
										{
											*phenotype_mem_p = MF_SHADOW_USE;
										}
								}

							if (!phenotype_p)
								{
									bson_oid_t *phenotype_id_p = GetNewUnitialisedBSONOid ();

									if (phenotype_id_p)
										{
											bson_oid_init_from_string (phenotype_id_p, oid_s);

											phenotype_p = GetMeasuredVariableById (phenotype_id_p, data_p);

											if (phenotype_p)
												{
													if (has_cache_flag)
														{
															if (AddMeasuredVariableToCache (data_p, phenotype_p, MF_SHALLOW_COPY))
																{
																	*phenotype_mem_p = MF_SHADOW_USE;
																}
															else
																{
																	PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "AddMeasuredVariableToCache () failed to cache MeasuredVariable for \"%s\"", oid_s);
																}
														}
												}
											else
												{
													PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, observation_json_p, "Failed to get phenotype from id \"%s\"", oid_s);
												}

											FreeBSONOid (phenotype_id_p);
										}		/* if (phenotype_id_p) */
									else
										{
											PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, observation_json_p, "Failed to allocate MeasuredVariable's BSONOid");
										}

								}		/* if (!phenotype_p) */

						}		/* if (oid_s) */
					else
						{
							PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, val_p, "GetNamedIdAsStringFromJSON () failed for key \"%s\"", MONGO_OID_KEY_S);
						}

				}		/* if (val_p) */
			else
				{
					PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, observation_json_p, "Failed to find any phenotype data from json");
				}

		}

	return phenotype_p;
}



const char *GetObservationTypeAsString (const ObservationType obs_type)
{
	const char *obs_type_s = NULL;

	if (obs_type < OT_NUM_TYPES)
		{
			obs_type_s = * (S_OBSERVATION_TYPES_SS + obs_type);
		}

	return obs_type_s;
}


bool DetermineObservationTypeFromString (const char * const obs_type_s, ObservationType *obs_type_p)
{
	ObservationType i;
	const char **type_ss = S_OBSERVATION_TYPES_SS;

	for (i = 0; i < OT_NUM_TYPES; ++ i, ++ type_ss)
		{
			if (strcmp (obs_type_s, *type_ss) == 0)
				{
					*obs_type_p = i;
					return true;
				}
		}

	return false;
}

