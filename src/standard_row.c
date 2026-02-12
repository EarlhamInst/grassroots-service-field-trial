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
 * row.c
 *
 *  Created on: 28 Sep 2018
 *      Author: billy
 */

#define ALLOCATE_STANDARD_ROW_TAGS (1)
#include "standard_row.h"

#include "memory_allocations.h"
#include "string_utils.h"
#include "streams.h"
#include "observation.h"
#include "dfw_util.h"
#include "plot_jobs.h"
#include "treatment_factor_value.h"
#include "frictionless_data_util.h"


static bool AddObservationsToJSON (json_t *row_json_p, LinkedList *observations_p, const ViewFormat format, const FieldTrialServiceData *data_p);

static bool GetObservationsFromJSON (const json_t *row_json_p, StandardRow *row_p, FieldTrialServiceData *data_p);

static bool GetTreatmentFactorValuesFromJSON (const json_t *row_json_p, StandardRow *row_p, const Study *study_p, const FieldTrialServiceData *data_p);

static bool AddTreatmentFactorsToJSON (json_t *row_json_p, LinkedList *treatment_factors_p, const Study *study_p, const ViewFormat format);

static bool IsFlagTrue (const json_t *json_p, const char * const key_s);


static void ClearStandardRow (Row *row_p);


static bool AddStandardRowFrictionlessDataDetails (const Row *base_row_p, json_t *row_fd_p, const FieldTrialServiceData *service_data_p, const char * const null_sequence_s);


static bool AddStandardRowFromJSON (Row *row_p, const json_t *row_json_p, const FieldTrialServiceData *data_p);

static StandardRow *AllocateEmptyStandardRow (void);


static StandardRow *AllocateEmptyStandardRow (void)
{
	LinkedList *observations_p = AllocateLinkedList (FreeObservationNode);

	if (observations_p)
		{
			LinkedList *tf_values_p = AllocateLinkedList (FreeTreatmentFactorValueNode);

			if (tf_values_p)
				{
					StandardRow *row_p = (StandardRow *) AllocMemory (sizeof (StandardRow));

					if (row_p)
						{
							row_p -> sr_observations_p = observations_p;
							row_p -> sr_treatment_factor_values_p = tf_values_p;

							row_p -> sr_material_p = NULL;
							row_p -> sr_material_mem = MF_ALREADY_FREED;
							row_p -> sr_replicate_index = 0;
							row_p -> sr_replicate_control_flag = false;
							row_p -> sr_store_code_s = NULL;


							SetRowCallbackFunctions (& (row_p -> sr_base),
																			 ClearStandardRow,
																			 AddStandardRowToJSON,
																			 NULL,
																			 AddStandardRowFrictionlessDataDetails);


							return row_p;
						}

					FreeLinkedList (tf_values_p);
				}

			FreeLinkedList (observations_p);
		}

	return NULL;
}


StandardRow *AllocateStandardRow (bson_oid_t *id_p, const uint32 rack_index, const uint32 study_index, const bool replicate_control_flag, const uint32 replicate, Material *material_p, MEM_FLAG material_mem, const char * const store_code_s, Plot *parent_plot_p)
{
	LinkedList *observations_p = AllocateLinkedList (FreeObservationNode);

	if (observations_p)
		{
			LinkedList *tf_values_p = AllocateLinkedList (FreeTreatmentFactorValueNode);

			if (tf_values_p)
				{
					char *copied_store_code_s = NULL;

					if ((store_code_s == NULL) || ((copied_store_code_s = EasyCopyToNewString (store_code_s)) != NULL))
						{
							StandardRow *row_p = (StandardRow *) AllocMemory (sizeof (StandardRow));

							if (row_p)
								{
									if (InitRow (& (row_p -> sr_base), id_p, study_index, parent_plot_p, RT_STANDARD,
															 ClearStandardRow,
															 AddStandardRowToJSON,
															 NULL,
															 AddStandardRowFrictionlessDataDetails))
										{
											row_p -> sr_rack_index = rack_index;
											row_p -> sr_material_p = material_p;
											row_p -> sr_material_mem = material_mem;
											row_p -> sr_observations_p = observations_p;
											row_p -> sr_treatment_factor_values_p = tf_values_p;
											row_p -> sr_replicate_index = replicate;
											row_p -> sr_replicate_control_flag = replicate_control_flag;
											row_p -> sr_store_code_s = copied_store_code_s;

											return row_p;
										}

									FreeMemory (row_p);
								}
							else
								{
									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate row " UINT32_FMT " at [" UINT32_FMT "," UINT32_FMT "]", study_index, parent_plot_p -> pl_row_index, parent_plot_p -> pl_column_index);
								}

							FreeCopiedString (copied_store_code_s);
						}		/* if (copied_store_code_s) */
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to copy store code %s for  " UINT32_FMT " at [" UINT32_FMT "," UINT32_FMT "]", store_code_s, study_index, parent_plot_p -> pl_row_index, parent_plot_p -> pl_column_index);
						}

					FreeLinkedList (tf_values_p);
				}		/* if (tf_values_p) */
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate treatment factors list " UINT32_FMT " at [" UINT32_FMT "," UINT32_FMT "]", study_index, parent_plot_p -> pl_row_index, parent_plot_p -> pl_column_index);
				}


			FreeLinkedList (observations_p);
		}
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate observations list " UINT32_FMT " at [" UINT32_FMT "," UINT32_FMT "]", study_index, parent_plot_p -> pl_row_index, parent_plot_p -> pl_column_index);
		}

	return NULL;
}


void ClearStandardRow (Row *row_p)
{
	StandardRow *standard_row_p = (StandardRow *) row_p;

	if (standard_row_p -> sr_store_code_s)
		{
			FreeCopiedString (standard_row_p -> sr_store_code_s);
		}

	FreeLinkedList (standard_row_p -> sr_treatment_factor_values_p);

	FreeLinkedList (standard_row_p -> sr_observations_p);

	if ((standard_row_p -> sr_material_mem == MF_DEEP_COPY) || (standard_row_p -> sr_material_mem == MF_SHALLOW_COPY))
		{
			if (standard_row_p -> sr_material_p)
				{
					FreeMaterial (standard_row_p -> sr_material_p);
				}
		}
}


void SetStandardRowMaterial (StandardRow *row_p, Material *material_p, MEM_FLAG material_mem)
{

}



StandardRow *GetStandardRowFromJSON (const json_t *row_json_p, Plot *plot_p, Material *material_p, const Study *study_p, const ViewFormat format, FieldTrialServiceData *data_p)
{
	StandardRow *row_p = NULL;
	Material *material_to_use_p = material_p;

	if (!plot_p)
		{
			bson_oid_t *plot_id_p = GetNewUnitialisedBSONOid ();

			if (plot_id_p)
				{
					if (GetNamedIdFromJSON (row_json_p, RO_PLOT_ID_S, plot_id_p))
						{
							plot_p = GetPlotById (plot_id_p, NULL, format, data_p);
						}

					FreeBSONOid (plot_id_p);
				}
		}

	if (plot_p)
		{
			row_p = AllocateEmptyStandardRow ();

			if (row_p)
				{
					if (PopulateRowFromJSON (& (row_p -> sr_base), plot_p, row_json_p, format, data_p))
						{
							if (format != VF_CLIENT_MINIMAL)
								{
									/*
									 * If we haven't already got the material, get it!
									 */
									if (!material_to_use_p)
										{
											bson_oid_t *material_id_p = GetNewUnitialisedBSONOid ();

											bool success_flag = true;

											if (material_id_p)
												{
													if (GetNamedIdFromJSON (row_json_p, SR_MATERIAL_ID_S, material_id_p))
														{
															material_to_use_p = GetMaterialById (material_id_p, data_p);

															if (material_to_use_p)
																{
																	success_flag = true;
																}
															else
																{
																	char id_s [MONGO_OID_STRING_BUFFER_SIZE];

																	bson_oid_to_string (material_id_p, id_s);
																	PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, row_json_p, "Failed to get material with id %s", id_s);
																}

														}		/* if (GetNamedIdFromJSON (json_p, SR_MATERIAL_ID_S, material_id_p)) */
													else
														{
															char plot_id_s [MONGO_OID_STRING_BUFFER_SIZE];

															bson_oid_to_string (plot_p -> pl_id_p, plot_id_s);

															PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, row_json_p, "Failed to get row's material id for plot \"%s\" at [" UINT32_FMT ", " UINT32_FMT "]", plot_id_s, plot_p -> pl_row_index, plot_p -> pl_column_index);
														}

													FreeBSONOid (material_id_p);
												}		/* if (material_id_p) */
											else
												{
													char id_s [MONGO_OID_STRING_BUFFER_SIZE];

													bson_oid_to_string (plot_p -> pl_id_p, id_s);
													PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate row's material id for plot \"%s\" at [" UINT32_FMT ", " UINT32_FMT "]", id_s, plot_p -> pl_row_index, plot_p -> pl_column_index);
												}

										}		/* if (!material_to_use_p) */

									if (material_to_use_p)
										{
											json_int_t rack_index = -1;

											if (GetJSONInteger (row_json_p, SR_RACK_INDEX_S, &rack_index))
												{
													bool rep_control_flag = false;
													uint32 replicate = 1;
													const json_t *rep_json_p = json_object_get (row_json_p, SR_REPLICATE_S);

													if (rep_json_p)
														{
															if (json_is_string (rep_json_p))
																{
																	const char *rep_s = json_string_value (rep_json_p);

																	if (rep_s)
																		{
																			if (Stricmp (rep_s, SR_REPLICATE_CONTROL_S) == 0)
																				{
																					rep_control_flag = true;
																				}
																			else
																				{
																					PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, row_json_p, "Invalid replicate value \"%s\"", rep_s);
																				}
																		}
																	else
																		{
																			PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, row_json_p, "Missing replicate value");
																		}
																}
															else if (json_is_integer (rep_json_p))
																{
																	replicate = json_integer_value (rep_json_p);
																}
														}

													if ((replicate != 0) || (rep_control_flag))
														{
															MEM_FLAG mf = material_to_use_p == material_p ? MF_SHADOW_USE : MF_SHALLOW_COPY;

															row_p -> sr_rack_index = rack_index;
															row_p -> sr_material_p = material_to_use_p;
															row_p -> sr_material_mem = mf;
															row_p -> sr_replicate_index = replicate;


															SetStandardRowGenotypeControl (row_p, rep_control_flag);


															if (row_p)
																{
																	const char *store_code_s = GetJSONString (row_json_p, SR_STORE_CODE_S);

																	if (SetStandardRowStoreCode (row_p, store_code_s))
																		{
																			if (GetObservationsFromJSON (row_json_p, row_p, data_p))
																				{
																					if (!GetTreatmentFactorValuesFromJSON (row_json_p, row_p, study_p, data_p))
																						{
																							PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, row_json_p, "GetTreatmentFactorValuesFromJSON failed");
																							FreeRow (& (row_p -> sr_base));
																							row_p = NULL;

																							/* id_p and material_to_use_p have been freed by FreeRow () */
																							material_to_use_p = NULL;
																						}
																				}
																			else
																				{
																					PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, row_json_p, "GetObservationsFromJSON failed");
																					FreeRow (& (row_p -> sr_base));
																					row_p = NULL;

																					/* id_p has been freed by FreeRow () */
																					material_to_use_p = NULL;
																				}

																		}		/* if (SetStandardRowStoreCode (row_p, store_code_s)) */
																	else
																		{
																			PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, row_json_p, "SetStandardRowStoreCode () failed");
																			FreeRow (& (row_p -> sr_base));
																			row_p = NULL;

																			/* id_p has been freed by FreeRow () */
																			material_to_use_p = NULL;
																		}

																}

														}		/* if ((replicate != 0) || (rep_control_flag)) */

												}		/* if (GetJSONInteger (json_p, SR_RACK_INDEX_S, &rack_index)) */

										}		/* if (material_to_use_p) */
									else
										{
											char id_s [MONGO_OID_STRING_BUFFER_SIZE];

											bson_oid_to_string (plot_p -> pl_id_p, id_s);
											PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to get row's material for plot \"%s\" at [" UINT32_FMT ", " UINT32_FMT "]", id_s, plot_p -> pl_row_index, plot_p -> pl_column_index);
										}

								}		/* if (format == VF_CLIENT_FULL) */

						}		/* if (PopulateRowFromJSON (& (row_p -> sr_base), plot_p, row_json_p, format, data_p)) */

				}		/* if (row_p) */

		}		/* if (plot_p) */




	if (row_p)
		{
			if (material_to_use_p && (material_to_use_p != material_p) && (row_p -> sr_material_p != material_to_use_p))
				{
					FreeMaterial (material_to_use_p);
				}

			return row_p;
		}
	else
		{
			if (material_to_use_p && (material_to_use_p != material_p))
				{
					FreeMaterial (material_to_use_p);
				}

			return NULL;
		}
}

//
//bool SaveRow (Row *row_p, const FieldTrialServiceData *data_p, bool insert_flag)
//{
//	bson_t *selector_p = NULL;
//	bool success_flag = false;
//
//	if (PrepareSaveData (& (row_p -> sr_id_p), &selector_p))
//		{
//			json_t *row_json_p = GetRowAsJSON (row_p, VF_STORAGE, NULL, data_p);
//
//			if (row_json_p)
//				{
//					if (SaveMongoData (data_p -> dftsd_mongo_p, row_json_p, data_p -> dftsd_collection_ss [DFTD_ROW], selector_p))
//						{
//							success_flag = true;
//						}
//					else
//						{
//							PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, row_json_p, "SaveMongoData failed");
//						}
//
//					json_decref (row_json_p);
//				}		/* if (row_json_p) */
//			else
//				{
//					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "GetRowAsJSON failed for row " UINT32_FMT " for plot [" UINT32_FMT ", " UINT32_FMT "] in study \"%s\"", row_p -> sr_rack_index, row_p -> sr_plot_p -> pl_row_index, row_p -> sr_plot_p -> pl_column_index, row_p -> sr_plot_p -> pl_parent_p -> st_name_s);
//				}
//
//		}		/* if (row_p -> sr_id_p) */
//	else
//		{
//			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "PrepareSaveData failed for row " UINT32_FMT " for plot [" UINT32_FMT ", " UINT32_FMT "] in study \"%s\"", row_p -> sr_rack_index, row_p -> sr_plot_p -> pl_row_index, row_p -> sr_plot_p -> pl_column_index, row_p -> sr_plot_p -> pl_parent_p -> st_name_s);
//		}
//
//	return success_flag;
//}
//




bool AddObservationToStandardRow (StandardRow *row_p, Observation *observation_p)
{
	bool success_flag = false;
	ObservationNode *node_p = NULL;

	/*
	 * If the observation has the same phenotype and date as an existing one,
	 * then replace it. If not, then simply add it.
	 */
	node_p = (ObservationNode *) (row_p -> sr_observations_p -> ll_head_p);
	while (node_p && (!success_flag))
		{
			Observation *existing_observation_p = node_p -> on_observation_p;

			if (observation_p == existing_observation_p)
				{
					success_flag = true;
				}
			else if (AreObservationsMatching (existing_observation_p, observation_p, true))
				{
					node_p -> on_observation_p = observation_p;
					FreeObservation (existing_observation_p);
					success_flag = true;
				}
			else
				{
					node_p = (ObservationNode *) (node_p -> on_node.ln_next_p);
				}
		}

	if (!success_flag)
		{
			node_p = AllocateObservationNode (observation_p);

			if (node_p)
				{
					LinkedListAddTail (row_p -> sr_observations_p, & (node_p -> on_node));
					success_flag = true;
				}
			else
				{
					char row_id_s [MONGO_OID_STRING_BUFFER_SIZE];
					char observation_id_s [MONGO_OID_STRING_BUFFER_SIZE];

					bson_oid_to_string (row_p -> sr_base.ro_id_p, row_id_s);
					bson_oid_to_string (observation_p -> ob_id_p, observation_id_s);

					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add observation \"%s\" to row \"%s\"", observation_id_s, row_id_s);
				}
		}

	return success_flag;
}



bool AddTreatmentFactorValueToStandardRow (StandardRow *row_p, TreatmentFactorValue *tf_value_p)
{
	bool success_flag = false;
	TreatmentFactorValueNode *node_p = NULL;

	/*
	 * If the treatment factor has the same treatment factor and date as an existing one,
	 * then replace it. If not, then simply add it.
	 */
	node_p = (TreatmentFactorValueNode *) (row_p -> sr_treatment_factor_values_p -> ll_head_p);
	while (node_p)
		{
			TreatmentFactorValue *existing_tf_value_p = node_p -> tfvn_value_p;

			if (AreTreatmentFactorValuesMatching (existing_tf_value_p, tf_value_p))
				{
					node_p -> tfvn_value_p = tf_value_p;
					FreeTreatmentFactorValue (existing_tf_value_p);
					success_flag = true;
					node_p = NULL;		/* force exit from loop */
				}
			else
				{
					node_p = (TreatmentFactorValueNode *) (node_p -> tfvn_node.ln_next_p);
				}
		}

	if (!success_flag)
		{
			node_p = AllocateTreatmentFactorValueNode (tf_value_p);

			if (node_p)
				{
					LinkedListAddTail (row_p -> sr_treatment_factor_values_p, & (node_p -> tfvn_node));
					success_flag = true;
				}
			else
				{
					char row_id_s [MONGO_OID_STRING_BUFFER_SIZE];
					char treatment_id_s [MONGO_OID_STRING_BUFFER_SIZE];
					const bson_oid_t *treatment_id_p = GetTreatmentIdForTreatmentFactorValue (tf_value_p);

					bson_oid_to_string (row_p -> sr_base.ro_id_p, row_id_s);
					bson_oid_to_string (treatment_id_p, treatment_id_s);

					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add observation \"%s\" to row \"%s\"", treatment_id_s, row_id_s);
				}
		}

	return success_flag;
}


void UpdateStandardRow (StandardRow *row_p, const uint32 rack_index, const bool replicate_control_flag, const uint32 replicate, Material *material_p, MEM_FLAG material_mem, const char * const store_code_s)
{
	if (row_p -> sr_rack_index != rack_index)
		{
			row_p -> sr_rack_index = rack_index;
		}

	if (row_p -> sr_material_p)
		{
			if (row_p -> sr_material_p -> ma_id_p != material_p -> ma_id_p)
				{
					if ((row_p -> sr_material_mem == MF_DEEP_COPY) || (row_p -> sr_material_mem == MF_SHALLOW_COPY))
						{
							if (row_p -> sr_material_p)
								{
									FreeMaterial (row_p -> sr_material_p);
								}
						}
				}

			row_p -> sr_material_p = material_p;
			row_p -> sr_material_mem = material_mem;
		}

	if (replicate_control_flag != IsStandardRowGenotypeControl (row_p))
		{
			SetStandardRowGenotypeControl (row_p, replicate_control_flag);
		}

	if (!replicate_control_flag)
		{
			if (row_p -> sr_replicate_index != replicate)
				{
					row_p -> sr_replicate_index = replicate;
				}
		}
}


static bool AddObservationsToJSON (json_t *row_json_p, LinkedList *observations_p, const ViewFormat format, const FieldTrialServiceData *data_p)
{
	bool success_flag = false;

	if (observations_p -> ll_size > 0)
		{
			json_t *observations_json_p = json_array ();

			if (observations_json_p)
				{
					if (json_object_set_new (row_json_p, SR_OBSERVATIONS_S, observations_json_p) == 0)
						{
							ObservationNode *node_p = (ObservationNode *) (observations_p -> ll_head_p);

							success_flag = true;

							while (node_p && success_flag)
								{
									const Observation *observation_p = node_p -> on_observation_p;
									json_t *observation_json_p = GetObservationAsJSON (observation_p, format, data_p);

									if (observation_json_p)
										{
											if (json_array_append_new (observations_json_p, observation_json_p) == 0)
												{
													node_p = (ObservationNode *) (node_p -> on_node.ln_next_p);
												}
											else
												{
													success_flag = false;
													json_decref (observation_json_p);
												}
										}
									else
										{
											success_flag = false;
										}
								}		/* while (node_p && success_flag) */

						}		/* if (json_object_set_new (row_json_p, SR_PHENOTYPES_S, phenotypes_array_p) == 0) */
					else
						{
							json_decref (observations_json_p);
						}

				}		/* if (phenotypes_array_p) */

		}
	else
		{
			success_flag = true;
		}

	return success_flag;
}



static bool GetObservationsFromJSON (const json_t *row_json_p, StandardRow *row_p, FieldTrialServiceData *data_p)
{
	bool success_flag = false;
	const json_t *observations_json_p = json_object_get (row_json_p, SR_OBSERVATIONS_S);

	/*
	 * Are there any observations?
	 */
	if (observations_json_p)
		{
			size_t size = json_array_size (observations_json_p);
			size_t i;

			success_flag = true;

			for (i = 0; i < size; ++ i)
				{
					const json_t *observation_json_p = json_array_get (observations_json_p, i);
					Observation *observation_p = GetObservationFromJSON (observation_json_p, data_p);

					if (observation_p)
						{
							if (!AddObservationToStandardRow (row_p, observation_p))
								{
									char row_id_s [MONGO_OID_STRING_BUFFER_SIZE];

									bson_oid_to_string (row_p -> sr_base.ro_id_p, row_id_s);
									PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, observation_json_p, "Failed to add observation to for row \"%s\"", row_id_s);

									FreeObservation (observation_p);
									success_flag = false;
									i = size;		/* force exit from loop */
								}

						}		/* if (observation_p) */
					else
						{
							char row_id_s [MONGO_OID_STRING_BUFFER_SIZE];

							bson_oid_to_string (row_p -> sr_base.ro_id_p, row_id_s);
							PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, observation_json_p, "GetObservationFromJSON failed for row \"%s\"", row_id_s);

							success_flag = false;
							i = size;		/* force exit from loop */
						}

				}		/* for (i = 0; i < size; ++ i) */

		}		/* if (observations_json_p) */
	else
		{
			success_flag = true;
		}

	return success_flag;
}


static bool AddTreatmentFactorsToJSON (json_t *row_json_p, LinkedList *treatment_factors_p, const Study *study_p, const ViewFormat format)
{
	bool success_flag = false;

	if (treatment_factors_p -> ll_size > 0)
		{
			json_t *treatment_factors_json_p = json_array ();

			if (treatment_factors_json_p)
				{
					if (json_object_set_new (row_json_p, SR_TREATMENTS_S, treatment_factors_json_p) == 0)
						{
							TreatmentFactorValueNode *node_p = (TreatmentFactorValueNode *) (treatment_factors_p -> ll_head_p);

							success_flag = true;

							while (node_p && success_flag)
								{
									const TreatmentFactorValue *tfv_p = node_p -> tfvn_value_p;
									json_t *treatment_factor_json_p = GetTreatmentFactorValueAsJSON (tfv_p, study_p);

									if (treatment_factor_json_p)
										{
											if (json_array_append_new (treatment_factors_json_p, treatment_factor_json_p) == 0)
												{
													node_p = (TreatmentFactorValueNode *) (node_p -> tfvn_node.ln_next_p);
												}
											else
												{
													success_flag = false;
													json_decref (treatment_factor_json_p);
												}
										}
									else
										{
											success_flag = false;
										}
								}		/* while (node_p && success_flag) */

						}		/* if (json_object_set_new (row_json_p, SR_PHENOTYPES_S, phenotypes_array_p) == 0) */
					else
						{
							json_decref (treatment_factors_json_p);
						}

				}		/* if (phenotypes_array_p) */

		}
	else
		{
			success_flag = true;
		}

	return success_flag;
}


static bool GetTreatmentFactorValuesFromJSON (const json_t *row_json_p, StandardRow *row_p, const Study *study_p, const FieldTrialServiceData *data_p)
{
	bool success_flag = false;
	const json_t *tf_values_json_p = json_object_get (row_json_p, SR_TREATMENTS_S);

	if (tf_values_json_p)
		{
			size_t size = json_array_size (tf_values_json_p);
			size_t i;

			success_flag = true;

			for (i = 0; i < size; ++ i)
				{
					const json_t *tf_value_json_p = json_array_get (tf_values_json_p, i);
					TreatmentFactorValue *tf_value_p = GetTreatmentFactorValueFromJSON (tf_value_json_p, study_p, data_p);

					if (tf_value_p)
						{
							if (!AddTreatmentFactorValueToStandardRow (row_p, tf_value_p))
								{
									char row_id_s [MONGO_OID_STRING_BUFFER_SIZE];

									bson_oid_to_string (row_p -> sr_base.ro_id_p, row_id_s);
									PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, tf_value_json_p, "Failed to add TreatmentFactorValue to for row \"%s\"", row_id_s);

									FreeTreatmentFactorValue (tf_value_p);
									success_flag = false;
									i = size;		/* force exit from loop */
								}

						}		/* if (observation_p) */
					else
						{
							char row_id_s [MONGO_OID_STRING_BUFFER_SIZE];

							bson_oid_to_string (row_p -> sr_base.ro_id_p, row_id_s);
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "GetTreatmentFactorValueFromJSON failed for row \"%s\"", row_id_s);


							success_flag = false;
							i = size;		/* force exit from loop */
						}

				}		/* for (i = 0; i < size; ++ i) */

		}		/* if (phenotypes_json_p) */
	else
		{
			success_flag = true;
		}

	return success_flag;
}


bool SetStandardRowStoreCode (StandardRow *row_p, const char * const store_code_s)
{
	bool success_flag = false;

	if (store_code_s)
		{
			char *copied_store_code_s = NULL;

			if (CopyAndAddStringValue (store_code_s, &copied_store_code_s))
				{
					if (row_p -> sr_store_code_s)
						{
							FreeCopiedString (row_p -> sr_store_code_s);
						}

					row_p -> sr_store_code_s = copied_store_code_s;
					success_flag = true;
				}
		}
	else
		{
			if (row_p -> sr_store_code_s)
				{
					FreeCopiedString (row_p -> sr_store_code_s);
				}

			row_p -> sr_store_code_s = NULL;
			success_flag = true;
		}

	return success_flag;
}



void SetStandardRowGenotypeControl (StandardRow *row_p, bool control_flag)
{
	row_p -> sr_replicate_control_flag = control_flag;
}


bool IsStandardRowGenotypeControl (const StandardRow *row_p)
{
	return row_p -> sr_replicate_control_flag;
}


bool AddStandardRowToPlotTable (const StandardRow *row_p, json_t *table_row_p, const FieldTrialServiceData *service_data_p)
{
	bool success_flag = false;

	if ((row_p -> sr_rack_index == 0) || (SetJSONInteger (table_row_p, PL_RACK_TITLE_S, row_p -> sr_rack_index)))
		{
			if ((row_p -> sr_replicate_index == 0) || (SetJSONInteger (table_row_p, PL_REPLICATE_TITLE_S, row_p -> sr_replicate_index)))
				{
					if (row_p -> sr_material_p)
						{
							if ((row_p -> sr_material_p -> ma_accession_s == NULL) || (SetJSONString (table_row_p, PL_ACCESSION_TABLE_TITLE_S, row_p -> sr_material_p -> ma_accession_s)))
								{
									GeneBank *gene_bank_p = GetGeneBankById (row_p -> sr_material_p -> ma_gene_bank_id_p, VF_CLIENT_MINIMAL, service_data_p);

									if (gene_bank_p)
										{
											if (SetJSONString (table_row_p, PL_GENE_BANK_S, gene_bank_p -> gb_name_s))
												{
													success_flag = true;
												}

											FreeGeneBank (gene_bank_p);
										}
								}
						}
					else
						{
							success_flag = true;
						}

				}		/* if ((row_p -> sr_replicate_index == 0) || (SetJSONInteger (table_row_p, PL_REPLICATE_TITLE_S, row_p -> sr_rephttps://www.theguardian.com/ukhttps://www.theguardian.com/uklicate_index))) */

		}		/* if ((row_p -> sr_rack_index == 0) || (SetJSONInteger (table_row_p, S_RACK_TITLE_S, row_p -> sr_rack_index))) */

	return success_flag;
}




static bool IsFlagTrue (const json_t *json_p, const char * const key_s)
{
	bool value_flag = false;

	GetJSONBoolean (json_p, key_s, &value_flag);

	return value_flag;
}



bool AddStandardRowToJSON (const Row *base_row_p, json_t *row_json_p, const ViewFormat format, const FieldTrialServiceData *data_p)
{
	bool success_flag = false;
	StandardRow *row_p = (StandardRow *) base_row_p;

	/*
	 * If there's a material, add it
	 */
	if (row_p -> sr_material_p)
		{
			switch (format)
			{
				case VF_CLIENT_FULL:
					{
						json_t *material_json_p = GetMaterialAsJSON (row_p -> sr_material_p, true, data_p);

						if (material_json_p)
							{
								if (json_object_set_new (row_json_p, SR_MATERIAL_S, material_json_p) == 0)
									{
										success_flag = true;
									}
								else
									{
										PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, material_json_p, "Failed to add material to row json");
										json_decref (material_json_p);
									}
							}
						else
							{
								PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "GetMaterialAsJSON failed for \"%s\"", row_p -> sr_material_p -> ma_accession_s);
							}
					}
					break;

				case VF_STORAGE:
					{
						if (AddNamedCompoundIdToJSON (row_json_p, row_p -> sr_material_p -> ma_id_p, SR_MATERIAL_ID_S))
							{
								success_flag = true;
							}
						else
							{
								PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, row_json_p, "Failed to add material \"%s\" to row json", row_p -> sr_material_p -> ma_accession_s);
							}
					}
					break;

				default:
					break;
			}
		}		/* if (row_p -> sr_material_p) */
	else
		{
			success_flag = true;
		}

	if (success_flag)
		{
			success_flag = false;

			if (SetJSONString (row_json_p, SR_STORE_CODE_S, row_p -> sr_store_code_s))
				{
					if (SetJSONInteger (row_json_p, SR_RACK_INDEX_S, row_p -> sr_rack_index))
						{
							const ViewFormat obs_format = (format == VF_STORAGE) ? VF_STORAGE : VF_CLIENT_MINIMAL;

							if (AddObservationsToJSON (row_json_p, row_p -> sr_observations_p, obs_format, data_p))
								{
									if (AddTreatmentFactorsToJSON (row_json_p, row_p -> sr_treatment_factor_values_p, row_p -> sr_base.ro_study_p, format))
										{
											if (row_p -> sr_replicate_control_flag)
												{
													if (SetJSONString (row_json_p, SR_REPLICATE_S, SR_REPLICATE_CONTROL_S))
														{
															success_flag = true;
														}
													else
														{
															char *id_s = GetBSONOidAsString (row_p -> sr_base.ro_id_p);

															if (id_s)
																{
																	PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, row_json_p, "SetJSONString () failed for \"%s\": \"%s\" in row \"%s\"", SR_REPLICATE_S, SR_REPLICATE_CONTROL_S, id_s);
																	FreeBSONOidString (id_s);
																}
															else
																{
																	PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, row_json_p, "SetJSONString () failed for \"%s\": \"%s\" in row", SR_REPLICATE_S, SR_REPLICATE_CONTROL_S);
																}
														}
												}
											else
												{

													if (SetJSONInteger (row_json_p, SR_REPLICATE_S, row_p -> sr_replicate_index))
														{
															success_flag = true;
														}
													else
														{
															char *id_s = GetBSONOidAsString (row_p -> sr_base.ro_id_p);

															if (id_s)
																{
																	PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, row_json_p, "SetJSONInteger () failed for \"%s\": " UINT32_FMT " in row \"%s\"", SR_REPLICATE_S, row_p -> sr_replicate_index, id_s);
																	FreeBSONOidString (id_s);
																}
															else
																{
																	PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, row_json_p, "SetJSONInteger () failed for \"%s\": " UINT32_FMT " in row", SR_REPLICATE_S, row_p -> sr_replicate_index);
																}
														}
												}

										}		/* if (AddTreatmentFactorsToJSON (row_json_p, row_p -> sr_treatment_factor_values_p, format)) */
									else
										{
											char *id_s = GetBSONOidAsString (row_p -> sr_base.ro_id_p);

											if (id_s)
												{
													PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, row_json_p, "AddTreatmentFactorsToJSON failed for row \"%s\"", id_s);
													FreeBSONOidString (id_s);
												}
											else
												{
													PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, row_json_p, "AddTreatmentFactorsToJSON failed for row");
												}

										}


								}		/* if (AddObservationsToJSON (row_json_p, row_p -> sr_observations_p, format)) */
							else
								{
									char *id_s = GetBSONOidAsString (row_p -> sr_base.ro_id_p);

									if (id_s)
										{
											PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, row_json_p, "AddObservationsToJSON failed for row \"%s\"", id_s);
											FreeBSONOidString (id_s);
										}
									else
										{
											PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, row_json_p, "AddObservationsToJSON failed for row");
										}

								}

						}		/* if (SetJSONInteger (row_json_p, SR_INDEX_S, row_p -> sr_index)) */
					else
						{
							PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, row_json_p, "Failed to add \"%s\": " UINT32_FMT, SR_RACK_INDEX_S, row_p -> sr_rack_index);
						}
				}
			else
				{
					char *id_s = GetBSONOidAsString (row_p -> sr_base.ro_id_p);

					if (id_s)
						{
							PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, row_json_p, "SetJSONString () failed for \"%s\": \"%s\" in row \"%s\"", SR_REPLICATE_S, SR_REPLICATE_CONTROL_S, id_s);
							FreeBSONOidString (id_s);
						}
					else
						{
							PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, row_json_p, "SetJSONString () failed for \"%s\": \"%s\" in row", SR_REPLICATE_S, SR_REPLICATE_CONTROL_S);
						}

				}



		}

	return success_flag;
}


static bool AddStandardRowFrictionlessDataDetails (const Row *base_row_p, json_t *row_fd_p, const FieldTrialServiceData *service_data_p, const char * const null_sequence_s)
{
	StandardRow *row_p = (StandardRow *) base_row_p;
	bool success_flag = false;

	if (SetJSONInteger (row_fd_p, PL_RACK_TITLE_S, row_p -> sr_rack_index))
		{
			if ((! (row_p -> sr_material_p)) || (SetJSONString (row_fd_p, PL_ACCESSION_TABLE_TITLE_S, row_p -> sr_material_p -> ma_accession_s)))
				{
					if (row_p -> sr_replicate_control_flag)
						{
							if (SetJSONString (row_fd_p, PL_REPLICATE_TITLE_S, SR_REPLICATE_CONTROL_S))
								{
									success_flag = true;
								}
							else
								{
									PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, row_fd_p, "Failed to add \"%s\": \"%s\"", PL_REPLICATE_TITLE_S, SR_REPLICATE_CONTROL_S);
								}
						}
					else
						{
							success_flag = SetJSONInteger (row_fd_p, PL_REPLICATE_TITLE_S, row_p -> sr_replicate_index);

							if (SetJSONInteger (row_fd_p, PL_REPLICATE_TITLE_S, row_p -> sr_replicate_index))
								{
									success_flag = true;
								}
							else
								{
									PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, row_fd_p, "Failed to add \"%s\": " UINT32_FMT, PL_REPLICATE_TITLE_S, row_p -> sr_replicate_index);
								}

						}

					if (success_flag)
						{
							/*
							 * Add the treatment factors
							 */

							if (row_p -> sr_treatment_factor_values_p)
								{
									uint32 num_added = 0;
									TreatmentFactorValueNode *tfv_node_p = (TreatmentFactorValueNode *) (row_p -> sr_treatment_factor_values_p -> ll_head_p);

									while (tfv_node_p && success_flag)
										{
											TreatmentFactorValue *tf_value_p = tfv_node_p -> tfvn_value_p;

											const char *url_s = GetTreatmentFactorUrl (tf_value_p -> tfv_factor_p);

											if (url_s)
												{
													if (SetFDTableString (row_fd_p, url_s, tf_value_p -> tfv_label_s, null_sequence_s))
														{
															++ num_added;
														}
													else
														{
															success_flag = false;
															PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, row_fd_p, "SetFDTableString () failed for url: \"%s\" label: \"%s\" null sequence: \"%s\"",
																								 url_s, tf_value_p -> tfv_label_s, null_sequence_s ? null_sequence_s : "NULL");
														}
												}
											else
												{
													success_flag = false;
													PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, row_fd_p, "GetTreatmentFactorUrl () failed for url: \"%s\" in study \"%s\"",
																						 tf_value_p -> tfv_label_s, tf_value_p -> tfv_factor_p -> tf_study_p -> st_name_s);
												}

											if (success_flag)
												{
													tfv_node_p = (TreatmentFactorValueNode *) (tfv_node_p -> tfvn_node.ln_next_p);
												}
										}

								}		/* if (row_p -> sr_treatment_factor_values_p) */


							if (success_flag)
								{
									if (row_p -> sr_observations_p)
										{
											uint32 num_added = 0;
											ObservationNode *obs_node_p = (ObservationNode *) (row_p -> sr_observations_p -> ll_head_p);

											while (obs_node_p && success_flag)
												{
													Observation *obs_p = obs_node_p -> on_observation_p;

													if (AddObservationValuesToFrictionlessData (obs_p, row_fd_p))
														{
															obs_node_p = (ObservationNode *) (obs_node_p -> on_node.ln_next_p);
															++ num_added;
														}
													else
														{
															char id_s [MONGO_OID_STRING_BUFFER_SIZE];

															success_flag = false;

															bson_oid_to_string (obs_p -> ob_id_p, id_s);
															PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, row_fd_p, "AddObservationValuesToFrictionlessData () failed for \"%s\"",
																								 id_s);
														}

												}		/* while (obs_node_p && success_flag) */

										}		/* if (row_p -> sr_observations_p) */


								}		/* if (success_flag) */

						}		/* if (success_flag) */

				}		/* if ((! (row_p -> sr_material_p)) || (SetJSONString (row_fd_p, PL_ACCESSION_TABLE_TITLE_S, row_p -> sr_material_p -> ma_accession_s))) */

		}		/* if (SetJSONInteger (row_fd_p, SR_RACK_INDEX_S, row_p -> sr_rack_index)) */
	else
		{
			PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, row_fd_p, "Failed to add \"%s\": " UINT32_FMT, SR_RACK_INDEX_S, row_p -> sr_rack_index);
		}

	return success_flag;
}

