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
 * phenotype.c
 *
 *  Created on: 17 Oct 2018
 *      Author: billy
 */


#define ALLOCATE_MEASURED_VARIABLE_TAGS (1)
#include "measured_variable.h"
#include "memory_allocations.h"
#include "string_utils.h"
#include "dfw_util.h"
#include "time_util.h"
#include "indexing.h"
#include "mongodb_util.h"

/*
 * static declarations
 */

static bool CreateInstrumentFromMeasuredVariableJSON (const json_t *phenotype_json_p, Instrument **instrument_pp, const FieldTrialServiceData *data_p);

static bool AppendSchemaTermToJSON (json_t *doc_p, const char * const key_s, const SchemaTerm *term_p);

static SchemaTerm *GetChildSchemTermFromJSON (const json_t *doc_p, const char * const key_s);

static bool AppendSchemaTermQuery (bson_t *query_p, const char *parent_key_s, const char *child_key_s, const char *child_value_s);


static bool AddCommonTermsToJSON (const MeasuredVariable *mv_p, json_t *phenotype_json_p);

static MeasuredVariable *GetMeasuredVariable (const bson_t *query_p, FieldTrialServiceData *data_p);

/*
static UnitTerm *GetUnitTermFromJSON (const json_t *phenotype_json_p, const FieldTrialServiceData *data_p)
{
	UnitTerm *unit_p = (UnitTerm *) AllocMemory (sizeof (UnitTerm));

	if (unit_p)
		{
			if (SetSchemaTermValues (& (unit_p -> ut_base_term), const char *url_s, const char *name_s, const char *description_s))
				{

				}

		}

}
 */


/*
 * API definitions
 */
MeasuredVariable *AllocateMeasuredVariable (bson_oid_t *id_p, SchemaTerm *trait_p, SchemaTerm *measurement_p, SchemaTerm *unit_p, SchemaTerm *variable_p, const ScaleClass *class_p,
																						CropOntology *parent_ontology_p, MEM_FLAG parent_ontology_mem)
{
	MeasuredVariable *treatment_p = NULL;
	CropOntology *ontology_to_use_p = NULL;

	/* do we need to copy the ontology? */
	if (parent_ontology_p && (parent_ontology_mem == MF_DEEP_COPY))
		{
			ontology_to_use_p = DuplicateCropOntology (parent_ontology_p);

			if (!ontology_to_use_p)
				{

				}
		}
	else
		{
			ontology_to_use_p = parent_ontology_p;
		}


	treatment_p = (MeasuredVariable *) AllocMemory (sizeof (MeasuredVariable));

	if (treatment_p)
		{
			treatment_p -> mv_id_p = id_p;
			treatment_p -> mv_trait_term_p = trait_p;
			treatment_p -> mv_measurement_term_p = measurement_p;
			treatment_p -> mv_unit_term_p = unit_p;
			treatment_p -> mv_variable_term_p = variable_p;
			treatment_p -> mv_scale_class_p = class_p;
			treatment_p -> mv_parent_p = ontology_to_use_p;
			treatment_p -> mv_parent_mem = parent_ontology_mem;


			return treatment_p;
		}		/* if (treatment_p) */

	return NULL;
}



void FreeMeasuredVariable (MeasuredVariable *measured_variable_p)
{
	if (measured_variable_p -> mv_id_p)
		{
			FreeBSONOid (measured_variable_p -> mv_id_p);
		}

	if (measured_variable_p -> mv_trait_term_p)
		{
			FreeSchemaTerm (measured_variable_p -> mv_trait_term_p);
		}

	if (measured_variable_p -> mv_measurement_term_p)
		{
			FreeSchemaTerm (measured_variable_p -> mv_measurement_term_p);
		}

	if (measured_variable_p -> mv_unit_term_p)
		{
			FreeSchemaTerm (measured_variable_p -> mv_unit_term_p);
		}

	if (measured_variable_p -> mv_variable_term_p)
		{
			FreeSchemaTerm (measured_variable_p -> mv_variable_term_p);
		}

	if (measured_variable_p -> mv_parent_p)
		{
			if ((measured_variable_p -> mv_parent_mem == MF_DEEP_COPY) || (measured_variable_p -> mv_parent_mem == MF_SHALLOW_COPY))
				{
					FreeCropOntology (measured_variable_p -> mv_parent_p);
				}

		}


	/*
	 * Still need to deallocate the instrument
	 *
	 * treatment_p -> mv_instrument_p
	 */

	FreeMemory (measured_variable_p);
}



json_t *GetMeasuredVariableAsJSON (const MeasuredVariable *mv_p, const ViewFormat format, const FieldTrialServiceData *data_p)
{
	json_t *phenotype_json_p = json_object ();

	if (phenotype_json_p)
		{
			if (AddDatatype (phenotype_json_p, DFTD_MEASURED_VARIABLE))
				{
					bool success_flag = false;



					if (format == VF_CLIENT_MINIMAL)
						{
							const char *variable_s = GetMeasuredVariableName (mv_p);

							if (SetJSONString (phenotype_json_p, MV_VARIABLE_S, variable_s))
								{
									success_flag = true;
								}
							else
								{
									PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, phenotype_json_p, MV_VARIABLE_S, variable_s);
								}

						}		/* if (format == VF_CLIENT_MINIMAL) */
					else
						{
							if (AddCommonTermsToJSON (mv_p, phenotype_json_p))
								{
									if (mv_p -> mv_scale_class_p)
										{
											json_t *scale_json_p = GetScaleClassAsJSON (mv_p -> mv_scale_class_p);

											if (scale_json_p)
												{
													if (json_object_set_new (phenotype_json_p, MV_SCALE_S, scale_json_p) == 0)
														{
															if (format == VF_STORAGE)
																{
																	if (AddCompoundIdToJSON (phenotype_json_p, mv_p -> mv_id_p))
																		{
																			if (mv_p -> mv_parent_p)
																				{
																					if (AddNamedCompoundIdToJSON (phenotype_json_p, mv_p -> mv_parent_p -> co_id_p, MV_ONTOLOGY_S))
																						{
																							success_flag = true;
																						}
																				}
																			else
																				{
																					success_flag = true;
																				}
																		}
																}
															else if ((format == VF_CLIENT_FULL) || (format == VF_INDEXING))
																{

																	/* Add the ontology details */
																	if (mv_p -> mv_parent_p)
																		{
																			json_t *ontology_json_p = GetCropOntologyAsJSON (mv_p -> mv_parent_p, format, data_p);

																			if (ontology_json_p)
																				{
																					if (json_object_set_new (phenotype_json_p, MV_ONTOLOGY_S, ontology_json_p) == 0)
																						{
																							if (format == VF_INDEXING)
																								{
																									if (AddCompoundIdToJSON (phenotype_json_p, mv_p -> mv_id_p))
																										{
																											success_flag = true;
																										}
																								}
																							else
																								{
																									success_flag = true;
																								}
																						}
																					else
																						{
																							json_decref (ontology_json_p);
																						}
																				}

																		}
																	else
																		{
																			success_flag = true;
																		}
																}
															else
																{
																	json_decref (scale_json_p);
																}

														}		/* if (json_object_set_new (phenotype_json_p, MV_SCALE_S, scale_json_p) == 0) */

												}		/* if (scale_json_p) */
											else
												{
													PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, phenotype_json_p, "AddCompoundIdToJSON () failed for \"%s\" to JSON", mv_p -> mv_variable_term_p -> st_url_s);
												}

										}		/* if (AddCommonTermsToJSON (mv_p, phenotype_json_p)) */

								}		/* if (AddCommonTermsToJSON (mv_p, phenotype_json_p)) */

						}		/* if (format == VF_CLIENT_MINIMAL) else ... */

					if (success_flag)
						{
							return phenotype_json_p;
						}		/* if (success_flag) */


				}		/* if (AddDatatype (phenotype_json_p, DFTD_MEASURED_VARIABLE)) */

			json_decref (phenotype_json_p);
		}		/* if (phenotype_json_p) */

	return NULL;
}


MeasuredVariable *GetMeasuredVariableFromJSON (const json_t *phenotype_json_p, const FieldTrialServiceData *data_p)
{
	SchemaTerm *trait_p = GetChildSchemTermFromJSON (phenotype_json_p, MV_TRAIT_S);

	if (trait_p)
		{
			SchemaTerm *unit_p = GetChildSchemTermFromJSON (phenotype_json_p, MV_UNIT_S);

			if (unit_p)
				{
					SchemaTerm *measurement_p = GetChildSchemTermFromJSON (phenotype_json_p, MV_MEASUREMENT_S);

					if (measurement_p)
						{
							SchemaTerm *variable_p = GetChildSchemTermFromJSON (phenotype_json_p, MV_VARIABLE_S);

							if (variable_p)
								{


								}

							/*
							 * The form is optional
							 */
							bool success_flag = true;
							SchemaTerm *form_p = NULL;
							const json_t *form_json_p = json_object_get (phenotype_json_p, MV_FORM_S);

							if (form_json_p)
								{
									form_p = GetSchemaTermFromJSON (form_json_p);

									if (!form_p)
										{
											success_flag = false;
											PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, form_json_p, "Failed to get SchemaTerm for form from JSON");
										}

								}		/* if (form_json_p) */

							if (success_flag)
								{
									MeasuredVariable *mv_p = NULL;
									bson_oid_t *id_p = GetNewUnitialisedBSONOid ();

									if (id_p)
										{
											if (GetMongoIdFromJSON (phenotype_json_p, id_p))
												{
													CropOntology *ontology_p = NULL;
													MEM_FLAG ontology_mem = MF_ALREADY_FREED;
													const ScaleClass *class_p = NULL;

													const json_t *scale_json_p = json_object_get (phenotype_json_p, MV_SCALE_S);
													bson_oid_t *ontology_id_p = GetNewUnitialisedBSONOid ();

													if (ontology_id_p)
														{
															if (GetNamedIdFromJSON (phenotype_json_p, MV_ONTOLOGY_S, ontology_id_p))
																{
																	ontology_p = GetCropOntologyById (ontology_id_p, data_p);

																	if (ontology_p)
																		{
																			ontology_mem = MF_SHALLOW_COPY;
																		}
																	else
																		{
																			/* error */
																		}

																}		/* if (GetNamedIdFromJSON (phenotype_json_p, MV_ONTOLOGY_S, ontology_id_p)) */

														}		/* if (ontology_id_p) */


													if (scale_json_p)
														{
															class_p = GetScaleClassFromJSON (scale_json_p);

															if (!class_p)
																{
																	PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, scale_json_p, "Failed to get scale class");
																}
														}

													mv_p = AllocateMeasuredVariable (id_p, trait_p, measurement_p, unit_p, variable_p, class_p, ontology_p, ontology_mem);

													if (mv_p)
														{
															return mv_p;
														}
													else
														{
															PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, phenotype_json_p, "Failed to allocate MeasuredVariable");
														}


												}		/* if (GetMongoIdFromJSON (phenotype_json_p, id_p)) */
											else
												{
													PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, phenotype_json_p, "Failed to get id \"%s\"", MONGO_ID_S);
												}

											FreeBSONOid (id_p);
										}		/* if (id_p) */
									else
										{
											PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, phenotype_json_p, "Failed to allocate id");
										}

								}		/* if (success_flag) */


							if (form_p)
								{
									FreeSchemaTerm (form_p);
								}

							if (variable_p)
								{
									FreeSchemaTerm (variable_p);
								}


							FreeSchemaTerm (measurement_p);
						}		/* if (measurement_p) */
					else
						{
							PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, phenotype_json_p, "Failed to get SchemaTerm for \"%s\" from JSON", MV_MEASUREMENT_S);
						}

					FreeSchemaTerm (unit_p);
				}		/* if (unit_p) */
			else
				{
					PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, phenotype_json_p, "Failed to get SchemaTerm for \"%s\" from JSON", MV_UNIT_S);
				}

			FreeSchemaTerm (trait_p);
		}		/* if (trait_p) */
	else
		{
			PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, phenotype_json_p, "Failed to get SchemaTerm for \"%s\" from JSON", MV_TRAIT_S);
		}

	return NULL;
}


OperationStatus SaveMeasuredVariable (MeasuredVariable *mv_p, ServiceJob *job_p, const char * const job_name_s, const FieldTrialServiceData *data_p)
{
	OperationStatus status = OS_FAILED;
	bson_t *selector_p = NULL;

	if (PrepareSaveData (& (mv_p -> mv_id_p), &selector_p))
		{
			json_t *phenotype_json_p = GetMeasuredVariableAsJSON (mv_p, VF_STORAGE, data_p);

			if (phenotype_json_p)
				{

					MongoTool *mongo_p = GetConfiguredMongoTool (data_p, NULL);

					if (mongo_p)
						{
							if (SaveAndBackupMongoDataWithTimestamp (mongo_p, phenotype_json_p, data_p -> dftsd_collection_ss [DFTD_MEASURED_VARIABLE],
																											 data_p -> dftsd_backup_collection_ss [DFTD_MEASURED_VARIABLE], DFT_BACKUPS_ID_KEY_S, selector_p, MONGO_TIMESTAMP_S))
								{
									json_t *index_json_p = GetMeasuredVariableAsJSON (mv_p, VF_INDEXING, data_p);

									if (index_json_p)
										{
											status = IndexData (job_p, index_json_p, job_name_s);

											if (status != OS_SUCCEEDED)
												{
													status = OS_PARTIALLY_SUCCEEDED;
													PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, index_json_p, "Failed to index Measured Variable \"%s\" as JSON to Lucene", mv_p -> mv_variable_term_p -> st_name_s);
													AddGeneralErrorMessageToServiceJob (job_p, "Measured Variable saved but failed to index for searching");
												}

											json_decref (index_json_p);
										}
									else
										{
											PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to index generate Measured Variable \"%s\" as JSON for Lucene", mv_p -> mv_variable_term_p -> st_name_s);
											AddGeneralErrorMessageToServiceJob (job_p, "Measured Variable saved but failed to index for searching");
										}

								}
							else
								{
									PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, phenotype_json_p, "SaveAndBackupMongoDataWithTimestamp () failed for Measured Variable \"%s\" as JSON to Lucene", mv_p -> mv_variable_term_p -> st_name_s);
									AddGeneralErrorMessageToServiceJob (job_p, "Failed to save Measured Variable");
								}


							FreeMongoTool (mongo_p);
						}		/* if (mongo_p) */
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "GetConfiguredMongoTool () failed");
						}




					json_decref (phenotype_json_p);
				}		/* if (phenotype_json_p) */
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "GetMeasuredVariableAsJSON () failed for \"%s\"", mv_p -> mv_variable_term_p -> st_name_s);
					AddGeneralErrorMessageToServiceJob (job_p, "Failed to save Measured Variable");
				}

		}		/* if (mv_p -> mv_id_p) */
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "PrepareSaveData () failed for Measured Variable \"%s\"", mv_p -> mv_variable_term_p -> st_name_s);
			AddGeneralErrorMessageToServiceJob (job_p, "Failed to save Measured Variable");
		}

	SetServiceJobStatus (job_p, status);

	return status;
}


static bool AppendSchemaTermQuery (bson_t *query_p, const char *parent_key_s, const char *child_key_s, const char *child_value_s)
{
	bool success_flag = false;
	char *compound_key_s = ConcatenateVarargsStrings (parent_key_s, ".", child_key_s, NULL);

	if (compound_key_s)
		{
			if (BSON_APPEND_UTF8 (query_p, compound_key_s, child_value_s))
				{
					success_flag = true;
				}

			FreeCopiedString (compound_key_s);
		}		/* if (compound_key_s) */

	return success_flag;
}


MeasuredVariable *GetMeasuredVariableBySchemaURLs (const char *trait_url_s, const char *method_url_s, const char *unit_url_s, const FieldTrialServiceData *data_p)
{
	MeasuredVariable *mv_p = NULL;
	bson_t *query_p = AllocateBSON ();

	if (query_p)
		{
			if (AppendSchemaTermQuery (query_p, MV_TRAIT_S, SCHEMA_TERM_URL_S, trait_url_s))
				{
					if (AppendSchemaTermQuery (query_p, MV_MEASUREMENT_S, SCHEMA_TERM_URL_S, method_url_s))
						{
							if (AppendSchemaTermQuery (query_p, MV_UNIT_S, SCHEMA_TERM_URL_S, unit_url_s))
								{
									mv_p = GetMeasuredVariable (query_p, data_p);
								}
						}
				}

			FreeBSON (query_p);
		}		/* if (query_p) */

	return mv_p;
}



MeasuredVariable *GetMeasuredVariableByIdString (const char *id_s, const FieldTrialServiceData *data_p)
{
	MeasuredVariable *treatment_p = NULL;

	if (bson_oid_is_valid (id_s, strlen (id_s)))
		{
			bson_oid_t oid;

			bson_oid_init_from_string (&oid, id_s);

			treatment_p = GetMeasuredVariableById (&oid, data_p);
		}

	return treatment_p;
}


MeasuredVariable *GetMeasuredVariableByName (const char * const name_s, const FieldTrialServiceData *data_p)
{
	MeasuredVariable *mv_p = NULL;
	char *key_s = ConcatenateVarargsStrings (MV_VARIABLE_S, ".", SCHEMA_TERM_NAME_S, NULL);

	if (key_s)
		{
			bson_t *query_p = BCON_NEW (key_s, BCON_UTF8 (name_s));

			if (query_p)
				{
					mv_p = GetMeasuredVariable (query_p, data_p);

					bson_free (query_p);
				}		/* if (query_p) */

			FreeCopiedString (key_s);
		}

	return mv_p;
}


MeasuredVariable *GetMeasuredVariableById (const bson_oid_t *phenotype_id_p, const FieldTrialServiceData *data_p)
{
	MeasuredVariable *mv_p = NULL;
	bson_t *query_p = BCON_NEW (MONGO_ID_S, BCON_OID (phenotype_id_p));

	if (query_p)
		{
			mv_p = GetMeasuredVariable (query_p, data_p);

			bson_free (query_p);
		}		/* if (query_p) */

	return mv_p;
}




const char *GetMeasuredVariableName (const MeasuredVariable *mv_p)
{
	const char *name_s = NULL;

	if (mv_p -> mv_variable_term_p)
		{
			name_s = mv_p -> mv_variable_term_p -> st_name_s;
		}

	return name_s;
}


const char *GetMeasuredVariableURL (const MeasuredVariable *mv_p)
{
	const char *name_s = NULL;

	if (mv_p -> mv_variable_term_p)
		{
			name_s = mv_p -> mv_variable_term_p -> st_url_s;
		}

	return name_s;
}






MeasuredVariableNode *AllocateMeasuredVariableNode (MeasuredVariable *variable_p, MEM_FLAG mv_mem)
{
	char *id_s = GetBSONOidAsString (variable_p -> mv_id_p);

	if (id_s)
		{
			MeasuredVariableNode *mv_node_p = (MeasuredVariableNode *) AllocMemory (sizeof (MeasuredVariableNode));

			if (mv_node_p)
				{
					InitListItem (& (mv_node_p -> mvn_node));

					mv_node_p -> mvn_measured_variable_p = variable_p;
					mv_node_p -> mvn_measured_variable_mem = mv_mem;
					mv_node_p -> mvn_id_s = id_s;

					return mv_node_p;
				}

			FreeBSONOidString (id_s);
		}
	else
		{
			const char *name_s = GetMeasuredVariableName (variable_p);
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "GetBSONOidAsString () failed for MeasuredVariable \"%s\"", name_s ? name_s : "");
		}

	return NULL;
}

void FreeMeasuredVariableNode (ListItem *node_p)
{
	MeasuredVariableNode *mv_node_p = (MeasuredVariableNode *) node_p;

	if (mv_node_p -> mvn_id_s)
		{
			FreeBSONOidString (mv_node_p -> mvn_id_s);
		}

	if (mv_node_p -> mvn_measured_variable_p)
		{
			if ((mv_node_p -> mvn_measured_variable_mem == MF_DEEP_COPY) || (mv_node_p -> mvn_measured_variable_mem == MF_SHALLOW_COPY))
				{
					FreeMeasuredVariable (mv_node_p -> mvn_measured_variable_p);
				}
		}

	FreeMemory (mv_node_p);
}


const ScaleClass *GetMeasuredVariableScaleClass (const MeasuredVariable * const variable_p)
{
	return variable_p -> mv_scale_class_p;
}


OperationStatus IndexMeasuredVariable (MeasuredVariable *mv_p, ServiceJob *job_p, const char *job_name_s, FieldTrialServiceData *data_p)
{
	OperationStatus status = OS_FAILED;
	const ViewFormat format = VF_INDEXING;

	json_t *mv_json_p = GetMeasuredVariableAsJSON (mv_p, format, data_p);

	if (mv_json_p)
		{
			status = IndexData (job_p, mv_json_p, job_name_s);

			if (status != OS_SUCCEEDED)
				{
					PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, mv_json_p, "Failed to index MeasuredVariable \"%s\" as JSON to Lucene", mv_p -> mv_variable_term_p -> st_name_s);
					AddGeneralErrorMessageToServiceJob (job_p, "Study saved but failed to index for searching");
				}

			json_decref (mv_json_p);
		}

	return status;
}


/*
 * static definitions
 */


static bool AppendSchemaTermToJSON (json_t *doc_p, const char * const key_s, const SchemaTerm *term_p)
{
	bool success_flag = false;
	json_t *term_json_p = GetSchemaTermAsJSON (term_p);

	if (term_json_p)
		{
			if (json_object_set_new (doc_p, key_s, term_json_p) == 0)
				{
					success_flag = true;
				}		/* if (json_object_set_new (doc_p, key_s, term_json_p) == 0) */
			else
				{
					PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, doc_p, "Failed to add SchemaTerm to JSON document");
					json_decref (term_json_p);
				}

		}		/* if (term_json_p) */
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to get SchemaTerm as JSON for \"%s\"", term_p -> st_url_s);
		}

	return success_flag;
}




static SchemaTerm *GetChildSchemTermFromJSON (const json_t *doc_p, const char * const key_s)
{
	const json_t *term_json_p = json_object_get (doc_p, key_s);

	if (term_json_p)
		{
			SchemaTerm *term_p = GetSchemaTermFromJSON (term_json_p);

			if (term_p)
				{
					return term_p;
				}
			else
				{
					PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, term_json_p, "Failed to get SchemaTerm from JSON");
				}
		}		/* if (term_json_p) */
	else
		{
			PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, term_json_p, "Failed to get \"%s\" for SchemaTerm child", key_s);
		}

	return NULL;
}



static bool AddCommonTermsToJSON (const MeasuredVariable *mv_p, json_t *phenotype_json_p)
{
	bool success_flag = false;

	if (AppendSchemaTermToJSON (phenotype_json_p, MV_VARIABLE_S, mv_p -> mv_variable_term_p))
		{
			if (AppendSchemaTermToJSON (phenotype_json_p, MV_TRAIT_S, mv_p -> mv_trait_term_p))
				{
					if (AppendSchemaTermToJSON (phenotype_json_p, MV_MEASUREMENT_S, mv_p -> mv_measurement_term_p))
						{
							if (AppendSchemaTermToJSON (phenotype_json_p, MV_UNIT_S, mv_p -> mv_unit_term_p))
								{
									success_flag = true;

								}		/* if (AddSchemTermToJSON (phenotype_json_p, MV_UNIT_S, mv_p -> mv_unit_term_p)) */
							else
								{
									PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, phenotype_json_p, "Failed to add SchemaTerm for \"%s\" to JSON", mv_p -> mv_unit_term_p -> st_url_s);
								}

						}		/* if (AddSchemTermToJSON (phenotype_json_p, MV_MEASUREMENT_S, mv_p -> mv_measurement_term_p)) */
					else
						{
							PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, phenotype_json_p, "Failed to add SchemaTerm for \"%s\" to JSON", mv_p -> mv_measurement_term_p -> st_url_s);
						}

				}		/* if (AddSchemTermToJSON (phenotype_json_p, MV_TRAIT_S, mv_p -> mv_trait_term_p)) */
			else
				{
					PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, phenotype_json_p, "Failed to add SchemaTerm for \"%s\" to JSON", mv_p -> mv_trait_term_p -> st_url_s);
				}

		}		/* if (AppendSchemaTermToJSON (phenotype_json_p, MV_VARIABLE_S, mv_p -> mv_variable_term_p)) */
	else
		{
			PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, phenotype_json_p, "Failed to add SchemaTerm for \"%s\" to JSON", mv_p -> mv_variable_term_p -> st_url_s);
		}

	return success_flag;
}


static MeasuredVariable *GetMeasuredVariable (const bson_t *query_p, FieldTrialServiceData *data_p)
{
	MeasuredVariable *phenotype_p = NULL;

	/*
	 * Get a new MongoTool in case we're being called from data_p's MongoTool as we don't want to overwrite
	 * its state, cursor, etc.
	 */
	MongoTool *mongo_p = GetConfiguredMongoTool (data_p, NULL);

	if (mongo_p)
		{
			if (SetMongoToolCollection (mongo_p, data_p -> dftsd_collection_ss [DFTD_MEASURED_VARIABLE]))
				{
					json_t *results_p = GetAllMongoResultsAsJSON (mongo_p, query_p, NULL);

					if (results_p)
						{
							if (json_is_array (results_p))
								{
									const size_t num_results = json_array_size (results_p);

									if (num_results == 1)
										{
											json_t *entry_p = json_array_get (results_p, 0);

											phenotype_p = GetMeasuredVariableFromJSON (entry_p, data_p);

											if (!phenotype_p)
												{
													PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, entry_p, "GetMeasuredVariableFromJSON () failed");
												}		/* if (!treatment_p) */

										}		/* if (num_results == 1) */
									else if (num_results == 0)
										{
											PrintBSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, query_p, "No results");
										}
									else
										{
											PrintBSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, query_p, SIZET_FMT " results", num_results);
										}

								}		/* if (json_is_array (results_p)) */
							else
								{
									PrintBSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, query_p, "Results is not an array");
									PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, results_p, "Results is not an array");
								}

							json_decref (results_p);
						}		/* if (results_p) */
					else
						{
							PrintBSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, query_p, "No Results");
						}


				}		/* if (SetMongoToolCollection (data_p -> dftsd_mongo_p, data_p -> dftsd_collection_ss [DFTD_PHENOTYPE])) */
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "SetMongoToolCollection () to phenotypes failed");
				}

			FreeMongoTool (mongo_p);
		}		/* if (mongo_p) */
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "GetConfiguredMongoTool () for phenotypes failed");
		}


	return phenotype_p;
}


