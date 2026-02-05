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
 * material_jobs.c
 *
 *  Created on: 23 Oct 2018
 *      Author: billy
 */


#include "material_jobs.h"
#include "string_utils.h"
#include "study_jobs.h"
#include "gene_bank.h"
#include "gene_bank_jobs.h"
#include "row_jobs.h"
#include "string_int_pair.h"
#include "row_processor.h"

#include "char_parameter.h"
#include "boolean_parameter.h"
#include "json_parameter.h"


/*
 * static declarations
 */

/*
static const char * const S_INTERNAL_NAME_TITLE_S = "Accession";
static const char * const S_PEDIGREE_TITLE_S = "Pedigree";
static const char * const S_BARCODE_TITLE_S = "Barcode";
static const char * const S_ACCESSION_TITLE_S = "Store code";
 */

static const char * const S_SPECIES_NAME_TITLE_S = "Species Name";
static const char * const S_GERMPLASM_ID_TITLE_S = "Germplasm ID";
static const char * const S_TYPE_TITLE_S = "Material Type";
static const char * const S_SELECTION_REASON_TITLE_S = "Selection Reason";
static const char * const S_GENERATION_TITLE_S = "Generation";
static const char * const S_SEED_SUPPLIER_TITLE_S = "Seed Supplier";
static const char * const S_SEED_SOURCE_TITLE_S = "Seed Source";
static const char * const S_GERMPLASM_ORIGIN_TITLE_S = "Germplasm Origin";
static const char * const S_IN_GRU_TITLE_S = "In GRU?";
static const char * const S_GRU_ACCESSION_TITLE_S = "GRU Accession";
static const char * const S_TGW_TITLE_S = "TGW";
static const char * const S_SEED_TREATMENT_TITLE_S = "Seed Treatment";
static const char * const S_CLEANED_TITLE_S = "Cleaned?";


static NamedParameterType S_MATERIAL_TABLE_COLUMN_DELIMITER = { "MA Data delimiter", PT_CHAR };
static NamedParameterType S_MATERIAL_TABLE = { "MA Upload", PT_JSON_TABLE};
static NamedParameterType S_STUDIES_LIST = { "MA Study", PT_STRING };
static NamedParameterType S_GENE_BANKS_LIST = { "MA Gene Bank", PT_STRING };

static NamedParameterType S_MATERIAL_ACCESSION = { "MA Accession", PT_STRING };
static NamedParameterType S_MATERIAL_ACCESSION_CASE_SENSITIVE = { "MA Accession case-sensitive", PT_BOOLEAN };
static NamedParameterType S_MATERIAL_SEARCH_STUDY_FORMAT = { "MA Search Study format", PT_BOOLEAN };

static const bool S_DEFAULT_SEARCH_CASE_SENSITIVITY_FLAG = true;


static json_t *GetTableParameterHints (void);

static Parameter *GetTableParameter (ParameterSet *param_set_p, ParameterGroup *group_p, const FieldTrialServiceData *data_p);

static bool AddMaterialsFromJSON (ServiceJob *job_p, const json_t *materials_json_p, Study *area_p, GeneBank *gene_bank_p, const FieldTrialServiceData *data_p);



/*
 * API definitions
 */

bool AddSubmissionMaterialParams (ServiceData *data_p, ParameterSet *param_set_p)
{
	bool success_flag = false;
	ParameterGroup *group_p = CreateAndAddParameterGroupToParameterSet ("Materials Submission", false, data_p, param_set_p);

	if (group_p)
		{
			Parameter *param_p = NULL;

			if ((param_p = EasyCreateAndAddStringParameterToParameterSet (data_p, param_set_p, group_p, S_STUDIES_LIST.npt_type, S_STUDIES_LIST.npt_name_s, "Study", "The available studies", NULL, PL_ALL)) != NULL)
				{
					const FieldTrialServiceData *dfw_service_data_p = (FieldTrialServiceData *) data_p;

					if (SetUpStudiesListParameter (dfw_service_data_p, param_p, NULL, false))
						{
							param_p = EasyCreateAndAddStringParameterToParameterSet (data_p, param_set_p, group_p, S_GENE_BANKS_LIST.npt_type, S_GENE_BANKS_LIST.npt_name_s, "Gene Bank", "The available gene banks", NULL, PL_ALL);

							if (param_p)
								{
									if (SetUpGenBanksListParameter ((FieldTrialServiceData *) data_p, (StringParameter *) param_p))
										{
											const char c = DFT_DEFAULT_COLUMN_DELIMITER;

											if ((param_p = EasyCreateAndAddCharParameterToParameterSet (data_p, param_set_p, group_p, S_MATERIAL_TABLE_COLUMN_DELIMITER.npt_name_s, "Delimiter", "The character delimiting columns", &c, PL_ADVANCED)) != NULL)
												{
													if ((param_p = GetTableParameter (param_set_p, group_p, dfw_service_data_p)) != NULL)
														{
															success_flag = true;
														}
												}

										}
								}
						}
				}
		}		/* if (group_p) */


	return success_flag;
}


bool RunForSubmissionMaterialParams (FieldTrialServiceData *data_p, ParameterSet *param_set_p, ServiceJob *job_p)
{
	bool job_done_flag = false;
	const json_t *materials_json_p = NULL;

	if (GetCurrentJSONParameterValueFromParameterSet (param_set_p, S_MATERIAL_TABLE.npt_name_s, &materials_json_p))
		{
			/*
			 * Has a spreadsheet been uploaded?
			 */
			if ((materials_json_p != NULL) && (json_array_size (materials_json_p) > 0))
				{
					OperationStatus status = OS_FAILED;
					const char *study_id_s = NULL;
					job_done_flag = true;

					if (GetCurrentStringParameterValueFromParameterSet (param_set_p, S_STUDIES_LIST.npt_name_s, &study_id_s))
						{
							Study *study_p = GetStudyByIdString (study_id_s, VF_STORAGE, data_p);

							if (study_p)
								{
									const char *gene_bank_id_s = NULL;

									if (GetCurrentStringParameterValueFromParameterSet (param_set_p, S_GENE_BANKS_LIST.npt_name_s, &gene_bank_id_s))
										{
											GeneBank *gene_bank_p = GetGeneBankByIdString (gene_bank_id_s, VF_STORAGE, data_p);

											if (gene_bank_p)
												{
													if (AddMaterialsFromJSON (job_p, materials_json_p, study_p, gene_bank_p, data_p))
														{
															status = OS_SUCCEEDED;
														}
													else
														{
															PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, materials_json_p, "AddMaterialsFromJSON for GeneBank with id \"%s\" and Study with id \"%s\"", gene_bank_id_s, study_id_s);
														}

													FreeGeneBank (gene_bank_p);
												}		/* if (gene_bank_p) */
											else
												{
													PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to get GeneBank with id \"%s\"", gene_bank_id_s);
												}

										}		/* if (GetParameterValueFromParameterSet (param_set_p, S_GENE_BANKS_LIST.npt_name_s, &value, true)) */
									else
										{
											PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to get \"%s\" parameter value", S_GENE_BANKS_LIST.npt_name_s);
										}

									FreeStudy (study_p);
								}		/* if (area_p) */
							else
								{
									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to get Study with id \"%s\"", study_id_s);
								}

						}		/* if (GetParameterValueFromParameterSet (param_set_p, S_STUDIES_LIST.npt_name_s, &value, true)) */
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to get \"%s\" parameter value", S_STUDIES_LIST.npt_name_s);
						}

					SetServiceJobStatus (job_p, status);
				}		/* if (! (IsStringEmpty (value.st_string_value_s))) */

		}		/* if (GetParameterValueFromParameterSet (param_set_p, S_ADD_EXPERIMENTAL_AREA.npt_name_s, &value, true)) */


	return job_done_flag;
}


bool GetSubmissionMaterialParameterTypeForNamedParameter (const char *param_name_s, ParameterType *pt_p)
{
	bool success_flag = true;

	if (strcmp (param_name_s, S_STUDIES_LIST.npt_name_s) == 0)
		{
			*pt_p = S_STUDIES_LIST.npt_type;
		}
	else if (strcmp (param_name_s, S_GENE_BANKS_LIST.npt_name_s) == 0)
		{
			*pt_p = S_GENE_BANKS_LIST.npt_type;
		}
	else if (strcmp (param_name_s, S_MATERIAL_TABLE.npt_name_s) == 0)
		{
			*pt_p = S_MATERIAL_TABLE.npt_type;
		}
	else if (strcmp (param_name_s, S_MATERIAL_TABLE_COLUMN_DELIMITER.npt_name_s) == 0)
		{
			*pt_p = S_MATERIAL_TABLE_COLUMN_DELIMITER.npt_type;
		}
	else
		{
			success_flag = false;
		}

	return success_flag;
}


bool AddSearchMaterialParams (ServiceData *data_p, ParameterSet *param_set_p)
{
	bool success_flag = false;
	const char * const group_name_s = "Materials Search";
	ParameterGroup *group_p = CreateAndAddParameterGroupToParameterSet (group_name_s, false, data_p, param_set_p);

	if (group_p)
		{
			Parameter *param_p = NULL;

			if ((param_p = EasyCreateAndAddStringParameterToParameterSet (data_p, param_set_p, group_p, S_MATERIAL_ACCESSION.npt_type, S_MATERIAL_ACCESSION.npt_name_s, "Accession", "Accession to search for", NULL, PL_ADVANCED)) != NULL)
				{
					const bool case_sensitivity_flag = S_DEFAULT_SEARCH_CASE_SENSITIVITY_FLAG;

					if ((param_p = EasyCreateAndAddBooleanParameterToParameterSet (data_p, param_set_p, group_p, S_MATERIAL_ACCESSION_CASE_SENSITIVE.npt_name_s, "Case sensitive", "Do a case-sensitive search for the accession", &case_sensitivity_flag, PL_ADVANCED)) != NULL)
						{
							if (AddStudyLevelDetailParameter (param_set_p, group_p, data_p))
								{
									success_flag = true;
								}
							else
								{
									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "AddStudyLevelDetailParameter () failed");
								}

						}		/* if ((param_p = EasyCreateAndAddBooleanParameterToParameterSet (data_p, param_set_p, group_p, S_MATERIAL_ACCESSION_CASE_SENSITIVE.npt_name_s, "Case sensitive", "Do a case-sensitive search for the accession", &case_sensitivity_flag, PL_ADVANCED)) != NULL) */
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add %s parameter", S_MATERIAL_ACCESSION_CASE_SENSITIVE.npt_name_s);
						}

				}		/* if ((param_p = EasyCreateAndAddStringParameterToParameterSet (data_p, param_set_p, group_p, S_MATERIAL_ACCESSION.npt_type, S_MATERIAL_ACCESSION.npt_name_s, "Accession", "Accession to search for", NULL, PL_ADVANCED)) != NULL) */
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add %s parameter", S_MATERIAL_ACCESSION.npt_name_s);
				}

		}		/* if (group_p) */
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "CreateAndAddParameterGroupToParameterSet () failed for %s", group_name_s);
		}

	return success_flag;
}


bool RunForSearchMaterialParams (FieldTrialServiceData *data_p, ParameterSet *param_set_p, ServiceJob *job_p)
{
	bool job_done_flag = false;
	const char *accession_s = NULL;


	if (GetCurrentStringParameterValueFromParameterSet (param_set_p, S_MATERIAL_ACCESSION.npt_name_s, &accession_s))
		{
			if (!IsStringEmpty (accession_s))
				{
					OperationStatus status = OS_FAILED_TO_START;
					bool case_senstive_search_flag = S_DEFAULT_SEARCH_CASE_SENSITIVITY_FLAG;
					GeneBank *gene_bank_p = NULL;
					Material *material_p = NULL;
					const bool *sens_flag_p = NULL;

					GetCurrentBooleanParameterValueFromParameterSet (param_set_p, S_MATERIAL_ACCESSION_CASE_SENSITIVE.npt_name_s, &sens_flag_p);

					if (sens_flag_p != NULL)
						{
							case_senstive_search_flag = *sens_flag_p;
						}

					material_p = GetMaterialByAccession (accession_s, gene_bank_p, case_senstive_search_flag, data_p);

					if (material_p)
						{
							ViewFormat format = VF_CLIENT_MINIMAL; // VF_CLIENT_FULL;

							GetStudyLevelDetailParameterValue (param_set_p, &format);

							status = GetAllStudiesContainingMaterial (material_p, job_p, format, data_p);

							FreeMaterial (material_p);
						}		/* if (material_p) */
					else
						{
							PrintErrors (STM_LEVEL_INFO, __FILE__, __LINE__, "Failed to find material \"%s\" for job \"%s\"", accession_s, job_p -> sj_name_s);
							status = OS_SUCCEEDED;
						}

					SetServiceJobStatus (job_p, status);
					job_done_flag = true;

				}		/* if (!IsStringEmpty (accession_s)) */

		}		/* if (GetParameterValueFromParameterSet (param_set_p, S_MATERIAL_ACCESSION_S.npt_name_s, &value, true)) */

	return job_done_flag;
}


bool GetSearchMaterialParameterTypeForNamedParameter (const char *param_name_s, ParameterType *pt_p)
{
	const NamedParameterType params [] =
		{
			S_MATERIAL_ACCESSION,
			S_MATERIAL_ACCESSION_CASE_SENSITIVE,
			NULL
		};

	return DefaultGetParameterTypeForNamedParameter (param_name_s, pt_p, params);
}



OperationStatus GetAllStudiesContainingMaterial (Material *material_p, ServiceJob *job_p, const ViewFormat format, FieldTrialServiceData *data_p)
{
	OperationStatus status = OS_FAILED;
	bool success_flag = true;

	char *query_key_s = ConcatenateVarargsStrings (PL_ROWS_S, ".", SR_MATERIAL_ID_S, NULL);

	if (query_key_s)
		{
			bson_t *query_p = BCON_NEW (query_key_s, BCON_OID (material_p -> ma_id_p));

			if (query_p)
				{

					MongoTool *mongo_p = GetConfiguredMongoTool (data_p, NULL);

					if (mongo_p)
						{
							if (SetMongoToolCollection (mongo_p, data_p -> dftsd_collection_ss [DFTD_PLOT]))
								{
									const char *fields_ss [2] = { PL_PARENT_STUDY_S, NULL };
									json_t *results_p = GetAllMongoResultsWithNamedFieldsAsJSON (mongo_p, query_p, fields_ss, NULL);

									if (results_p)
										{
											json_t *studies_cache_p = json_object ();

											if (studies_cache_p)
												{
													if (json_is_array (results_p))
														{
															const size_t num_results = json_array_size (results_p);
															size_t i = 0;

															while ((i < num_results) && success_flag)
																{
																	json_t *plot_json_p = json_array_get (results_p, i);
																	bson_oid_t oid;

																	/*
																	 * Get the study id
																	 */
																	if (GetNamedIdFromJSON (plot_json_p, PL_PARENT_STUDY_S, &oid))
																		{
																			char *id_s = GetBSONOidAsString (&oid);

																			if (id_s)
																				{
																					json_int_t count = 0;

																					GetJSONInteger (studies_cache_p, id_s, &count);

																					++ count;

																					if (!SetJSONInteger (studies_cache_p, id_s, count))
																						{
																							success_flag = false;
																						}

																					FreeBSONOidString (id_s);
																				}		/* if (id_s) */

																		}

																	if (success_flag)
																		{
																			++ i;
																		}

																}		/* while ((i < num_results) && success_flag) */


															if (success_flag)
																{
																	/*
																	 * Now we sort the studies by how many times the material appears
																	 */
																	size_t num_studies = json_object_size (studies_cache_p);

																	PrintLog (STM_LEVEL_INFO, __FILE__, __LINE__, "Found %u studies containing \"%s\"", num_studies, material_p -> ma_accession_s);

																	if (num_studies > 0)
																		{
																			StringIntPairArray *ids_with_counts_p = AllocateStringIntPairArray (num_studies);

																			if (ids_with_counts_p)
																				{
																					uint32 added_count = 0;
																					const char *key_s;
																					json_t *value_p;
																					JSONProcessor *processor_p = AllocateRowProcessor (material_p);
																					StringIntPair *pair_p = ids_with_counts_p -> sipa_values_p;

																					json_object_foreach (studies_cache_p, key_s, value_p)
																						{
																							int count = json_integer_value (value_p);

																							if (!SetStringIntPair (pair_p, (char *) key_s, MF_SHADOW_USE, count))
																								{
																									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "");
																								}

																							++ pair_p;
																						}		/* json_object_foreach (studies_cache_p, key_s, value_p) */

																					/*
																					 * Sort by the counts of how many times each material appears
																					 * in each study
																					 */
																					SortStringIntPairsByCountDescending (ids_with_counts_p);

																					for (i = num_studies, pair_p = ids_with_counts_p -> sipa_values_p; i > 0; -- i, ++ pair_p)
																						{
																							Study *study_p = GetStudyByIdString (pair_p -> sip_string_s, format, data_p);

																							if (study_p)
																								{
																									if (AddStudyToServiceJob (job_p, study_p, /*format*/VF_REFERENCE, processor_p, data_p))
																										{
																											++ added_count;
																										}
																									else
																										{
																											PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add study \"%s\" to results for job \"%s\"", study_p -> st_name_s, job_p -> sj_name_s);
																										}

																								}		/* if (study_p) */
																							else
																								{
																									FreeStudy (study_p);
																								}

																						}		/* for ( ; num_studies > 0; -- num_studies, ++ key_ss) */

																					if (processor_p)
																						{
																							FreeJSONProcessor (processor_p);
																						}

																					if (added_count == num_studies)
																						{
																							status = OS_SUCCEEDED;
																						}
																					else if (added_count > 0)
																						{
																							status = OS_PARTIALLY_SUCCEEDED;
																						}
																					else
																						{
																							status = OS_FAILED;
																						}

																					FreeStringIntPairArray (ids_with_counts_p);
																				}		/* if (ids_with_counts_p) */

																		}		/* if (studies_cache_p -> ht_size > 0) */

																}		/* if (success_flag) */

														}		/* if (json_is_array (results_p)) */

													json_decref (studies_cache_p);
												}		/* if (studies_cache_p */

											json_decref (results_p);
										}		/* if (results_p) */

								}		/* if (SetMongoToolCollection (data_p -> dftsd_mongo_p, data_p -> dftsd_collection_ss [DFTD_ROW] */


							FreeMongoTool (mongo_p);
						}		/* if (mongo_p) */
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "GetConfiguredMongoTool () failed");
						}




					bson_free (query_p);
				}		/* if (query_p) */

			FreeCopiedString (query_key_s);
		}		/* if (query_key_s) */

	return status;
}


static StringIntPairArray *SortStudies (json_t *studies_cache_p)
{
	const size_t num_studies = json_object_size (studies_cache_p);
	StringIntPairArray *ids_with_counts_p = AllocateStringIntPairArray (num_studies);

	if (ids_with_counts_p)
		{
			uint32 added_count = 0;
			const char *key_s;
			json_t *value_p;
			StringIntPair *pair_p = ids_with_counts_p -> sipa_values_p;

			json_object_foreach (studies_cache_p, key_s, value_p)
				{
					int count = json_integer_value (value_p);

					if (!SetStringIntPair (pair_p, (char *) key_s, MF_SHADOW_USE, count))
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "");
						}

					++ pair_p;
				}		/* json_object_foreach (studies_cache_p, key_s, value_p) */

			/*
			 * Sort by the counts of how many times each material appears
			 * in each study
			 */
			SortStringIntPairsByCountDescending (ids_with_counts_p);
		}

	return ids_with_counts_p;
}

/*
 * static definitions
 */


static json_t *GetTableParameterHints (void)
{
	json_t *hints_p = json_array ();

	if (hints_p)
		{
			if (AddColumnParameterHint (S_SPECIES_NAME_TITLE_S, NULL, PT_STRING, true, hints_p))
				{
					if (AddColumnParameterHint (S_GERMPLASM_ID_TITLE_S, NULL, PT_STRING, true, hints_p))
						{
							if (AddColumnParameterHint (S_TYPE_TITLE_S, NULL, PT_STRING, false, hints_p))
								{
									if (AddColumnParameterHint (S_SELECTION_REASON_TITLE_S, NULL, PT_STRING, false, hints_p))
										{
											if (AddColumnParameterHint (S_GENERATION_TITLE_S, NULL, PT_STRING, false, hints_p))
												{
													if (AddColumnParameterHint (S_SEED_SUPPLIER_TITLE_S, NULL, PT_STRING, false, hints_p))
														{
															if (AddColumnParameterHint (S_SEED_SOURCE_TITLE_S, NULL, PT_STRING, false, hints_p))
																{
																	if (AddColumnParameterHint (S_GERMPLASM_ORIGIN_TITLE_S, NULL, PT_STRING, false, hints_p))
																		{
																			if (AddColumnParameterHint (S_IN_GRU_TITLE_S, NULL, PT_BOOLEAN, true, hints_p))
																				{
																					if (AddColumnParameterHint (S_GRU_ACCESSION_TITLE_S, NULL, PT_STRING, false, hints_p))
																						{
																							if (AddColumnParameterHint (S_TGW_TITLE_S, NULL, PT_UNSIGNED_INT, false, hints_p))
																								{
																									if (AddColumnParameterHint (S_SEED_TREATMENT_TITLE_S, NULL, PT_STRING, false, hints_p))
																										{
																											if (AddColumnParameterHint (S_CLEANED_TITLE_S, NULL, PT_BOOLEAN, false, hints_p))
																												{
																													return hints_p;
																												}
																										}
																								}
																						}
																				}
																		}
																}
														}
												}
										}
								}
						}
				}

			json_decref (hints_p);
		}

	return NULL;
}


static Parameter *GetTableParameter (ParameterSet *param_set_p, ParameterGroup *group_p, const FieldTrialServiceData *data_p)
{
	Parameter *param_p = EasyCreateAndAddJSONParameterToParameterSet (& (data_p -> dftsd_base_data), param_set_p, group_p, S_MATERIAL_TABLE.npt_type, S_MATERIAL_TABLE.npt_name_s, "Material data to upload", "The data to upload", NULL, PL_ALL);

	if (param_p)
		{
			bool success_flag = false;
			json_t *hints_p = GetTableParameterHints ();

			if (hints_p)
				{
					if (AddParameterKeyJSONValuePair (param_p, PA_TABLE_COLUMN_HEADINGS_S, hints_p))
						{
							const char delim_s [2] = { DFT_DEFAULT_COLUMN_DELIMITER, '\0' };

							if (AddParameterKeyStringValuePair (param_p, PA_TABLE_COLUMN_DELIMITER_S, delim_s))
								{
									success_flag = true;
								}
						}

					json_decref (hints_p);
				}

			if (!success_flag)
				{
					FreeParameter (param_p);
					param_p = NULL;
				}

		}		/* if (param_p) */

	return param_p;
}




static bool AddMaterialsFromJSON (ServiceJob *job_p, const json_t *materials_json_p, Study *area_p, GeneBank *gene_bank_p, const FieldTrialServiceData *data_p)
{
	bool success_flag	= true;
	OperationStatus status = OS_FAILED;

	if (json_is_array (materials_json_p))
		{
			const size_t num_rows = json_array_size (materials_json_p);
			size_t i;
			size_t num_imported = 0;

			for (i = 0; i < num_rows; ++ i)
				{
					json_t *table_row_json_p = json_array_get (materials_json_p, i);


					const char *species_name_s = GetJSONString (table_row_json_p, S_SPECIES_NAME_TITLE_S);

					if (!IsStringEmpty (species_name_s))
						{
							const char *germplasm_id_s = GetJSONString (table_row_json_p, S_GERMPLASM_ID_TITLE_S);

							if (!IsStringEmpty (germplasm_id_s))
								{
									const char *type_s = GetJSONString (table_row_json_p, S_TYPE_TITLE_S);

									if (!IsStringEmpty (type_s))
										{
											const char *reason_s = GetJSONString (table_row_json_p, S_SELECTION_REASON_TITLE_S);

											if (!IsStringEmpty (reason_s))
												{
													const char *generation_s = GetJSONString (table_row_json_p, S_GENERATION_TITLE_S);

													if (!IsStringEmpty (generation_s))
														{
															const char *supplier_s = GetJSONString (table_row_json_p, S_SEED_SUPPLIER_TITLE_S);

															if (!IsStringEmpty (supplier_s))
																{
																	const char *source_s = GetJSONString (table_row_json_p, S_SEED_SOURCE_TITLE_S);

																	if (!IsStringEmpty (source_s))
																		{
																			const char *germplasm_origin_s = GetJSONString (table_row_json_p, S_GERMPLASM_ORIGIN_TITLE_S);

																			if (!IsStringEmpty (germplasm_origin_s))
																				{
																					bool in_gru_flag;

																					if (GetJSONBoolean (table_row_json_p, S_IN_GRU_TITLE_S, &in_gru_flag))
																						{
																							const char *accession_s = GetJSONString (table_row_json_p, S_GRU_ACCESSION_TITLE_S);

																							if (!IsStringEmpty (accession_s))
																								{
																									uint32 tgw;

																									if (GetJSONUnsignedInteger (table_row_json_p, S_TGW_TITLE_S, &tgw))
																										{
																											const char *treatment_s = GetJSONString (table_row_json_p, S_SEED_TREATMENT_TITLE_S);

																											if (!IsStringEmpty (treatment_s))
																												{
																													bool cleaned_flag;

																													if (GetJSONBoolean (table_row_json_p, S_CLEANED_TITLE_S, &cleaned_flag))
																														{
																															Material *material_p = NULL; // AllocateMaterial (NULL, accession_s, species_name_s, type_s, reason_s, generation_s, supplier_s, source_s, germplasm_origin_s, treatment_s, in_gru_flag, cleaned_flag, tgw, area_p, gene_bank_p -> gb_id_p, data_p);

																															if (material_p)
																																{
																																	if (SaveMaterial (material_p, data_p))
																																		{
																																			++ num_imported;
																																		}
																																	else
																																		{
																																			PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, table_row_json_p, "Failed to save Material");
																																			success_flag = false;
																																		}

																																	FreeMaterial (material_p);
																																}		/* if (material_p) */

																														}		/* if (GetJSONBoolean (table_row_json_p, S_CLEANED_TITLE_S, &cleaned_flag)) */
																													else
																														{
																															PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, table_row_json_p, "Failed to get \"%s\"", S_CLEANED_TITLE_S);
																														}

																												}		/* if (!IsStringEmpty (treatment_s)) */
																											else
																												{
																													PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, table_row_json_p, "Failed to get \"%s\"", S_CLEANED_TITLE_S);
																												}
																										}		/* if (GetJSONInteger (table_row_json_p, S_TGW_TITLE_S, (int *) &tgw)) */
																									else
																										{
																											PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, table_row_json_p, "Failed to get \"%s\"", S_TGW_TITLE_S);
																										}

																								}		/* if (!IsStringEmpty (accession_s)) */
																							else
																								{
																									PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, table_row_json_p, "Failed to get \"%s\"", S_GRU_ACCESSION_TITLE_S);
																								}

																						}
																					else
																						{
																							PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, table_row_json_p, "Failed to get \"%s\"", S_CLEANED_TITLE_S);
																						}

																				}		/* if (!IsStringEmpty (germplasm_origin_s)) */
																			else
																				{
																					PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, table_row_json_p, "Failed to get \"%s\"", S_GERMPLASM_ORIGIN_TITLE_S);
																				}

																		}		/* if (!IsStringEmpty (source_s)) */
																	else
																		{
																			PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, table_row_json_p, "Failed to get \"%s\"", S_SEED_SOURCE_TITLE_S);
																		}

																}		/* if (!IsStringEmpty (supplier_s)) */
															else
																{
																	PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, table_row_json_p, "Failed to get \"%s\"", S_SEED_SUPPLIER_TITLE_S);
																}

														}		/* if (!IsStringEmpty (generation_s)) */
													else
														{
															PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, table_row_json_p, "Failed to get \"%s\"", S_GENERATION_TITLE_S);
														}

												}		/* if (!IsStringEmpty (reason_s)) */
											else
												{
													PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, table_row_json_p, "Failed to get \"%s\"", S_SELECTION_REASON_TITLE_S);
												}

										}		/* if (!IsStringEmpty (type_s)) */
									else
										{
											PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, table_row_json_p, "Failed to get \"%s\"", S_TYPE_TITLE_S);
										}

								}		/* if (!IsStringEmpty (germplasm_id_s)) */
							else
								{
									PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, table_row_json_p, "Failed to get \"%s\"", S_GERMPLASM_ID_TITLE_S);
								}

						}		/* if (!IsStringEmpty (species_name_s)) */
					else
						{
							PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, table_row_json_p, "Failed to get \"%s\"", S_SPECIES_NAME_TITLE_S);
						}
				}		/* for (i = 0; i < num_rows; ++ i) */


			if (num_imported == num_rows)
				{
					status = OS_SUCCEEDED;
				}
			else if (num_imported > 0)
				{
					status = OS_PARTIALLY_SUCCEEDED;
				}

		}		/* if (json_is_array (plots_json_p)) */

	SetServiceJobStatus (job_p, status);

	return success_flag;
}



typedef struct MaterialsHighlighter
{
	JSONProcessor mh_base_processor;
	json_t *materials_to_highlight_json_p;
} MaterialsHighlighter;


static json_t *HighlightRowsWithMaterial (struct JSONProcessor *processor_p, struct Plot *plot_p, ViewFormat format, const FieldTrialServiceData *service_data_p)
{
	MaterialsHighlighter *highlighter_p = (MaterialsHighlighter *) processor_p;

	return NULL;
}


Material *GetOrCreateMaterial (const json_t *plot_data_p, GeneBank *gene_bank_p, const FieldTrialServiceData *data_p)
{
	Material *material_p = NULL;

	return material_p;
}



