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
 * material.c
 *
 *  Created on: 10 Oct 2018
 *      Author: billy
 */

#define ALLOCATE_MATERIAL_TAGS (1)
#include "material.h"
#include "memory_allocations.h"
#include "string_utils.h"
#include "gene_bank.h"
#include "dfw_util.h"
#include "mongodb_util.h"


static bool ReplaceMaterialField (const char *new_value_s, char **value_ss);

static Material *SearchForMaterial (bson_t *query_p, const FieldTrialServiceData *data_p);

static bool SetValidJSONString (json_t *material_json_p, const char *key_s, const char *value_s);

static char *GetRegex (const char *accession_s);

/*
 * API FUNCTIONS
 */




Material *AllocateMaterialByAccession (bson_oid_t *id_p, const char *accession_s, bson_oid_t *gene_bank_id_p, const FieldTrialServiceData *data_p)
{
	char *copied_accession_s = NULL;

	if ((copied_accession_s = EasyCopyToNewString (accession_s)) != NULL)
		{
			Material *material_p = (Material *) AllocMemory (sizeof (Material));

			if (material_p)
				{
					material_p -> ma_id_p = id_p;
					material_p -> ma_gene_bank_id_p = gene_bank_id_p;
					material_p -> ma_accession_s = copied_accession_s;

					return material_p;
				}		/* if (material_p) */
			else
				{
					char *gene_bank_id_s = GetBSONOidAsString (gene_bank_id_p);

					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate Material with accession \"%s\" in gene bank \"%s\"", accession_s, gene_bank_id_s ? gene_bank_id_s : "");

					if (gene_bank_id_s)
						{
							FreeBSONOidString (gene_bank_id_s);
						}
				}

			FreeCopiedString (copied_accession_s);
		}
	else
		{
			char *gene_bank_id_s = GetBSONOidAsString (gene_bank_id_p);

			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to copy accession \"%s\" for Material in gene bank \"%s\"", accession_s, gene_bank_id_s ? gene_bank_id_s : "");

			if (gene_bank_id_s)
				{
					FreeBSONOidString (gene_bank_id_s);
				}

		}

	return NULL;
}


void FreeMaterial (Material *material_p)
{
	if (material_p -> ma_id_p)
		{
			FreeBSONOid (material_p -> ma_id_p);
		}

	/*
	if (material_p -> ma_gene_bank_id_p)
		{
			FreeBSONOid (material_p -> ma_gene_bank_id_p);
		}
	*/

	if (material_p -> ma_accession_s)
		{
			FreeCopiedString (material_p -> ma_accession_s);
		}

	/*
	if (material_p -> ma_species_name_s)
		{
			FreeCopiedString (material_p -> ma_species_name_s);
		}

	if (material_p -> ma_type_s)
		{
			FreeCopiedString (material_p -> ma_type_s);
		}

	if (material_p -> ma_selection_reason_s)
		{
			FreeCopiedString (material_p -> ma_selection_reason_s);
		}

	if (material_p -> ma_generation_s)
		{
			FreeCopiedString (material_p -> ma_generation_s);
		}

	if (material_p -> ma_seed_supplier_s)
		{
			FreeCopiedString (material_p -> ma_seed_supplier_s);
		}

	if (material_p -> ma_germplasm_origin_s)
		{
			FreeCopiedString (material_p -> ma_germplasm_origin_s);
		}

	if (material_p -> ma_seed_source_s)
		{
			FreeCopiedString (material_p -> ma_seed_source_s);
		}

	if (material_p -> ma_seed_treatment_s)
		{
			FreeCopiedString (material_p -> ma_seed_treatment_s);
		}
 	*/


	FreeMemory (material_p);
}


json_t *GetMaterialAsJSON (const Material *material_p, const ViewFormat format, const FieldTrialServiceData *data_p)
{
	json_t *material_json_p = json_object ();

	if (material_json_p)
		{
			bool success_flag = true;

			if (format == VF_STORAGE)
				{
					if (AddCompoundIdToJSON (material_json_p, material_p -> ma_id_p))
						{
						}		/* if (AddCompoundIdToJSON (material_json_p, material_p -> ma_id_p)) */
					else
						{
							char id_s [MONGO_OID_STRING_BUFFER_SIZE];

							bson_oid_to_string (material_p -> ma_id_p, id_s);
							PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, material_json_p, "Failed to add compound id for \"%s\": \"%s\"", MONGO_ID_S, id_s);
							success_flag = false;
						}
				}

			if (success_flag)
				{
					if (SetJSONString (material_json_p, MA_ACCESSION_S, material_p -> ma_accession_s))
						{
							if (AddDatatype (material_json_p, DFTD_MATERIAL))
								{
									success_flag = false;

									if (material_p -> ma_gene_bank_id_p)
										{
											if (format == VF_CLIENT_FULL)
												{
													GeneBank *gene_bank_p = GetGeneBankById (material_p -> ma_gene_bank_id_p, format, data_p);

													if (gene_bank_p)
														{
															json_t *gene_bank_json_p = GetGeneBankAsJSON (gene_bank_p, format, material_p -> ma_accession_s);

															if (gene_bank_json_p)
																{
																	if (json_object_set_new (material_json_p, MA_GENE_BANK_S, gene_bank_json_p) == 0)
																		{
																			success_flag = true;
																		}
																	else
																		{
																			PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, material_json_p, "Failed to add gene bank json to material");
																			json_decref (gene_bank_json_p);
																		}
																}
															else
																{
																	PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to get gene bank \"%s\"", gene_bank_p -> gb_name_s);
																}

															FreeGeneBank (gene_bank_p);
														}
													else
														{
															char id_s [MONGO_OID_STRING_BUFFER_SIZE];

															bson_oid_to_string (material_p -> ma_gene_bank_id_p, id_s);
															PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to find gene bank with id \"%s\"", id_s);
														}
												}
											else
												{
													success_flag = AddNamedCompoundIdToJSON (material_json_p, material_p -> ma_gene_bank_id_p, MA_GENE_BANK_ID_S);
												}

										}		/* if (material_p -> ma_gene_bank_id_p) */
									else
										{
											success_flag = true;
										}

									if (success_flag)
										{
											return material_json_p;
										}

								}		/* if (AddDatatype (material_json_p, DFTD_MATERIAL)) */
							else
								{
									PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, material_json_p, "Failed to add datatype for material");
								}


							}		/* if (SetJSONString (material_json_p, MA_ACCESSION_S, material_p -> ma_accession_s) */
						else
							{
								PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, material_json_p, "Failed to add \"%s\": \"%s\"", MA_ACCESSION_S, material_p -> ma_accession_s);
							}


				}

			json_decref (material_json_p);
		}		/* if (material_json_p) */
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate JSON object for Material \"%s\"", material_p -> ma_accession_s);
		}

	return NULL;
}





Material *GetMaterialFromJSON (const json_t *json_p, const ViewFormat format, const FieldTrialServiceData *data_p)
{
	const char *accession_s = GetJSONString (json_p, MA_ACCESSION_S);

	if (accession_s)
		{
			bson_oid_t *id_p = GetNewUnitialisedBSONOid ();

			if (id_p)
				{
					if (GetMongoIdFromJSON (json_p, id_p))
						{
							bson_oid_t *gene_bank_id_p = GetNewUnitialisedBSONOid ();

							if (gene_bank_id_p)
								{
									if (GetNamedIdFromJSON (json_p, MA_GENE_BANK_ID_S, gene_bank_id_p))
										{
											bool success_flag = false;

											if (format == VF_CLIENT_FULL)
												{
												}
											else
												{
													success_flag = true;
												}

											if (success_flag)
												{
													Material *material_p = AllocateMaterialByAccession (id_p, accession_s, gene_bank_id_p, data_p);

													if (material_p)
														{
															return material_p;
														}
													else
														{
															PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, json_p, "Failed to allocate Material");
														}
												}

										}		/* if (GetNamedIdFromJSON (json_p, MA_GENE_BANK_ID_S, germplasm_id_p)) */
									else
										{
											PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, json_p, "Failed to get \"%s\"", MA_GENE_BANK_ID_S);
										}

									FreeBSONOid (gene_bank_id_p);
								}		/* if (gene_bank_id_p) */
							else
								{
									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate id for \"%s\"", MA_GENE_BANK_ID_S);
								}

						}		/* if (GetMongoIdFromJSON (json_p, id_p)) */
					else
						{
							PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, json_p, "Failed to get \"%s\"", MONGO_ID_S);
						}

					FreeBSONOid (id_p);
				}		/* if (id_p) */
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate id for \"%s\"", MONGO_ID_S);
				}

		}		/* if (accession_s) */
	else
		{
			PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, json_p, "Failed to get \"%s\"", MA_ACCESSION_S);
		}

	return NULL;
}


bool SaveMaterial (Material *material_p, const FieldTrialServiceData *data_p)
{
	bson_t *selector_p = NULL;
	bool success_flag = PrepareSaveData (& (material_p -> ma_id_p), &selector_p);

	if (success_flag)
		{
			json_t *material_json_p = GetMaterialAsJSON (material_p, false, data_p);

			if (material_json_p)
				{
					MongoTool *mongo_p = GetConfiguredMongoTool (data_p, NULL);

					if (mongo_p)
						{
							success_flag = SaveAndBackupMongoDataWithTimestamp (mongo_p, material_json_p, data_p -> dftsd_collection_ss [DFTD_MATERIAL],
								data_p -> dftsd_backup_collection_ss [DFTD_MATERIAL], DFT_BACKUPS_ID_KEY_S, selector_p, MONGO_TIMESTAMP_S);

							FreeMongoTool (mongo_p);
						}		/* if (mongo_p) */
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "GetConfiguredMongoTool () failed");
						}

					json_decref (material_json_p);
				}		/* if (material_json_p) */

		}		/* if (material_p -> ma_id_p) */

	return success_flag;
}




Material *GetOrCreateMaterialByAccession (const char *accession_s, GeneBank *gene_bank_p, const FieldTrialServiceData *data_p)
{
	Material *material_p = GetMaterialByAccession (accession_s, gene_bank_p, true, data_p);

	if (!material_p)
		{
			material_p = AllocateMaterialByAccession (NULL, accession_s, gene_bank_p -> gb_id_p, data_p);

			if (material_p)
				{
					if (!SaveMaterial (material_p, data_p))
						{
							FreeMaterial (material_p);
							material_p = NULL;

							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to save Material with internal name of \"%s\" for gene bank \"%s\"", accession_s, gene_bank_p -> gb_name_s);
						}
				}
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate Material with internal name of \"%s\" for gene bank \"%s\"", accession_s, gene_bank_p -> gb_name_s);
				}

		}		/* if (!material_p) */

	return material_p;
}




Material *GetMaterialByAccession (const char *accession_s, GeneBank *gene_bank_p, const bool case_sensitive_flag, const FieldTrialServiceData *data_p)
{
	Material *material_p = NULL;
	bson_t *query_p = bson_new ();

	if (query_p)
		{
			bool success_flag = false;

			if (case_sensitive_flag)
				{
					success_flag = BSON_APPEND_UTF8 (query_p, MA_ACCESSION_S, accession_s);
				}
			else
				{
					char *regex_s = GetRegex (accession_s);

					if (regex_s)
						{
							success_flag = BSON_APPEND_REGEX (query_p, MA_ACCESSION_S, regex_s, "i");

							FreeCopiedString (regex_s);
						}
				}

			if (success_flag)
				{
					if (gene_bank_p)
						{
							success_flag = BSON_APPEND_OID (query_p, MA_GENE_BANK_ID_S, gene_bank_p -> gb_id_p);
						}
				}


			if (success_flag)
				{
					material_p = SearchForMaterial (query_p, data_p);

					if (!material_p)
						{
							PrintBSONToErrors (STM_LEVEL_INFO, __FILE__, __LINE__, query_p, "SearchForMaterial did not find accession \"%s\" in gene bank \"%s\"", accession_s, gene_bank_p ? gene_bank_p -> gb_name_s : "");
						}

					bson_destroy (query_p);

				}		/* if (success_flag) */
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "GetMaterialByAccession could not generate query for accession \"%s\" in gene bank \"%s\"", accession_s, gene_bank_p ? gene_bank_p -> gb_name_s : "");
				}

		}		/* if (query_p) */
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "GetMaterialByAccession could not create query for accession \"%s\" in gene bank \"%s\"", accession_s, gene_bank_p ? gene_bank_p -> gb_name_s : "");
		}


	return material_p;
}


Material *GetMaterialById (const bson_oid_t *material_id_p, const FieldTrialServiceData *data_p)
{
	Material *material_p = NULL;
	bson_t *query_p = BCON_NEW (MONGO_ID_S, BCON_OID (material_id_p));

	if (query_p)
		{
			material_p = SearchForMaterial (query_p, data_p);

			if (!material_p)
				{
					char *id_s = GetBSONOidAsString (material_id_p);

					PrintBSONToErrors (STM_LEVEL_INFO, __FILE__, __LINE__, query_p, "SearchForMaterial did not find accession \"%s\"", id_s ? id_s : "");

					if (id_s)
						{
							FreeBSONOidString (id_s);
						}
				}

			bson_destroy (query_p);

		}		/* if (query_p) */
	else
		{
			char *id_s = GetBSONOidAsString (material_id_p);

			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "GetMaterialById could not create query for id \"%s\"", id_s);

			if (id_s)
				{
					FreeBSONOidString (id_s);
				}
		}

	return material_p;
}




static Material *SearchForMaterial (bson_t *query_p, const FieldTrialServiceData *data_p)
{
	Material *material_p = NULL;
	MongoTool *mongo_p = GetConfiguredMongoTool (data_p, NULL);

	if (mongo_p)
		{
			if (SetMongoToolCollection (mongo_p, data_p -> dftsd_collection_ss [DFTD_MATERIAL]))
				{
					json_t *results_p = GetAllMongoResultsAsJSON (mongo_p, query_p, NULL);

					if (results_p)
						{
							if (json_is_array (results_p))
								{
									if (json_array_size (results_p) == 1)
										{
											json_t *result_p = json_array_get (results_p, 0);

											material_p = GetMaterialFromJSON (result_p, VF_STORAGE, data_p);

											if (!material_p)
												{
													PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, result_p, "Failed to get Material from JSON");
												}

										}		/* if (json_array_size (results_p) == 1) */
									else
										{
											PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, results_p, "Materials array does not contain just a single item");
										}

								}		/* if (json_is_array (results_p)) */
							else
								{
									PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, results_p, "results are not an array");
								}

							json_decref (results_p);
						}		/* if (results_p) */
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "No results returned");
						}
				}		/* if (SetMongoToolCollection (data_p -> dftsd_mongo_p, data_p -> dftsd_collection_ss [DFTD_MATERIAL])) */
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to set mongo collection to \"%s\"", data_p -> dftsd_collection_ss [DFTD_MATERIAL]);
				}


			FreeMongoTool (mongo_p);
		}		/* if (mongo_p) */
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "GetConfiguredMongoTool () failed");
		}




	return material_p;
}


bool IsMaterialComplete (const Material * const material_p)
{
	return ((material_p -> ma_gene_bank_id_p) && (material_p -> ma_accession_s));
}


bool SetMaterialAccession (Material *material_p, const char * const accession_s)
{
	return ReplaceMaterialField (accession_s, & (material_p -> ma_accession_s));
}

/*
bool SetMaterialPedigree (Material *material_p, const char * const pedigree_s)
{
	return ReplaceMaterialField (pedigree_s, & (material_p -> ma_pedigree_s));
}


bool SetMaterialBarcode (Material *material_p, const char * const barcode_s)
{
	return ReplaceMaterialField (barcode_s, & (material_p -> ma_barcode_s));
}


bool SetMaterialInternalName (Material *material_p, const char * const internal_name_s)
{
	return ReplaceMaterialField (internal_name_s, & (material_p -> ma_internal_name_s));
}
*/

static bool ReplaceMaterialField (const char *new_value_s, char **value_ss)
{
	bool success_flag = false;

	if (new_value_s)
		{
			char *copied_value_s = EasyCopyToNewString (new_value_s);

			if (copied_value_s)
				{
					if (*value_ss)
						{
							FreeCopiedString (*value_ss);
							*value_ss = copied_value_s;
							success_flag = true;
						}
				}
		}
	else
		{
			if (*value_ss)
				{
					FreeCopiedString (*value_ss);
					*value_ss = NULL;
					success_flag = true;
				}
		}

	return success_flag;
}


static bool SetValidJSONString (json_t *material_json_p, const char *key_s, const char *value_s)
{
	return ((IsStringEmpty (value_s)) || (SetJSONString (material_json_p, key_s, value_s)));
}


static char *GetRegex (const char *accession_s)
{
	char *regex_s = NULL;
	char *lower_case_accession_s = GetStringAsLowerCase (accession_s);

	if (lower_case_accession_s)
		{
			regex_s = ConcatenateVarargsStrings ("^", lower_case_accession_s, "$", NULL);
		}

	return regex_s;
}

