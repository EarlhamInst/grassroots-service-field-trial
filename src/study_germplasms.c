/*
 * study_germplasms.c
 *
 *  Created on: 5 Nov 2019
 *      Author: billy
 */

#include "streams.h"
#include "string_utils.h"
#include "material.h"


Material *AllocateMaterial (bson_oid_t *id_p, const char *accession_s, const char *full_accession_s, const char *species_s, const char *type_s, const char *selection_reason_s, const char *generation_s, const char *supplier_s, const char *source_s, const char *germplasm_origin_s, const char *treatment_s, bool gru_flag, bool cleaned_flag, uint32 tgw, const Study *area_p, const bson_oid_t *gene_bank_id_p, const FieldTrialServiceData *data_p)
{
	char *copied_accession_s = NULL;

	if ((IsStringEmpty (accession_s)) || ((copied_accession_s = EasyCopyToNewString (accession_s)) != NULL))
		{
			char *copied_species_s = NULL;

			if ((IsStringEmpty (species_s)) || ((copied_species_s = EasyCopyToNewString (species_s)) != NULL))
				{
					char *copied_type_s = NULL;

					if ((IsStringEmpty (type_s)) || ((copied_type_s = EasyCopyToNewString (type_s)) != NULL))
						{
							char *copied_selection_reason_s = NULL;

							if ((IsStringEmpty (selection_reason_s)) || ((copied_selection_reason_s = EasyCopyToNewString (selection_reason_s)) != NULL))
								{
									char *copied_generation_s = NULL;

									if ((IsStringEmpty (generation_s)) || ((copied_generation_s = EasyCopyToNewString (generation_s)) != NULL))
										{
											char *copied_supplier_s = NULL;

											if ((IsStringEmpty (supplier_s)) || ((copied_supplier_s = EasyCopyToNewString (supplier_s)) != NULL))
												{
													char *copied_source_s = NULL;

													if ((IsStringEmpty (source_s)) || ((copied_source_s = EasyCopyToNewString (source_s)) != NULL))
														{
															char *copied_germplasm_origin_s = NULL;

															if ((IsStringEmpty (germplasm_origin_s)) || ((copied_germplasm_origin_s = EasyCopyToNewString (germplasm_origin_s)) != NULL))
																{
																	char *copied_treatment_s = NULL;

																	if ((IsStringEmpty (treatment_s)) || ((copied_treatment_s = EasyCopyToNewString (treatment_s)) != NULL))
																		{
																			bson_oid_t *copied_gene_bank_id_p = CopyBSONOid (gene_bank_id_p);

																			if (copied_gene_bank_id_p)
																				{
																					char *copied_accession_full_name_s = NULL;

																					if ((IsStringEmpty (full_accession_s)) || ((copied_accession_full_name_s = EasyCopyToNewString (full_accession_s)) != NULL))
																						{
																							Material *material_p = (Material *) AllocMemory (sizeof (Material));


																							if (material_p)
																								{
																									material_p -> ma_id_p = id_p;
																									material_p -> ma_accession_s = copied_accession_s;
																									material_p -> ma_gene_bank_id_p = copied_gene_bank_id_p;

																									/*
																									material_p -> ma_generation_s = copied_generation_s;
																									material_p -> ma_selection_reason_s = copied_selection_reason_s;
																									material_p -> ma_seed_supplier_s = copied_supplier_s;
																									material_p -> ma_seed_source_s = copied_source_s;
																									material_p -> ma_type_s = copied_type_s;
																									material_p -> ma_seed_treatment_s = copied_treatment_s;
																									material_p -> ma_species_name_s = copied_species_s;
																									material_p -> ma_germplasm_origin_s = copied_germplasm_origin_s;
																									material_p -> ma_parent_area_p = area_p;

																									material_p -> ma_cleaned_flag = cleaned_flag;
																									material_p -> ma_in_gru_flag = gru_flag;
																									material_p -> ma_tgw = tgw;
																									 */


																									return material_p;
																								}		/* if (material_p) */

																							if (copied_accession_full_name_s)
																								{
																									FreeCopiedString (copied_accession_full_name_s);
																								}
																						}
																					else
																						{
																							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to copy full accession \"%s\"", full_accession_s);
																						}

																					FreeBSONOid (copied_gene_bank_id_p);
																				}

																			if (copied_treatment_s)
																				{
																					FreeCopiedString (copied_treatment_s);
																				}
																		}		/* if ((IsStringEmpty (treatment_s)) || ((copied_generation_s = EasyCopyToNewString (treatment_s)) != NULL)) */
																	else
																		{
																			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to copy treatment \"%s\"", treatment_s);
																		}

																	if (copied_germplasm_origin_s)
																		{
																			FreeCopiedString (copied_germplasm_origin_s);
																		}
																}		/* if ((IsStringEmpty (germplasm_origin_s)) || ((copied_selection_reason_s = EasyCopyToNewString (germplasm_origin_s)) != NULL)) */
															else
																{
																	PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to copy germplasm origin \"%s\"", germplasm_origin_s);
																}

															if (copied_source_s)
																{
																	FreeCopiedString (copied_source_s);
																}
														}		/* if ((IsStringEmpty (source_s)) || ((copied_type_s = EasyCopyToNewString (source_s)) != NULL)) */
													else
														{
															PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to copy source \"%s\"", source_s);
														}

													if (copied_supplier_s)
														{
															FreeCopiedString (copied_supplier_s);
														}
												}		/* if ((IsStringEmpty (supplier_s)) || ((copied_species_s = EasyCopyToNewString (supplier_s)) != NULL)) */
											else
												{
													PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to copy supplier \"%s\"", supplier_s);
												}

											if (copied_generation_s)
												{
													FreeCopiedString (copied_generation_s);
												}
										}		/* if ((IsStringEmpty (generation_s)) || ((copied_generation_s = EasyCopyToNewString (generation_s)) != NULL)) */
									else
										{
											PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to copy generation \"%s\"", generation_s);
										}


									if (copied_selection_reason_s)
										{
											FreeCopiedString (copied_selection_reason_s);
										}
								}		/* if ((IsStringEmpty (selection_reason_s)) || ((copied_selection_reason_s = EasyCopyToNewString (selection_reason_s)) != NULL)) */
							else
								{
									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to copy selection reason \"%s\"", selection_reason_s);
								}

							if (copied_type_s)
								{
									FreeCopiedString (copied_type_s);
								}
						}		/* if ((IsStringEmpty (pedigree_s)) || ((copied_type_s = EasyCopyToNewString (pedigree_s)) != NULL)) */
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to copy type \"%s\"", type_s);
						}

					if (copied_species_s)
						{
							FreeCopiedString (copied_species_s);
						}
				}		/* if ((IsStringEmpty (species_s)) || ((copied_species_s = EasyCopyToNewString (species_s)) != NULL)) */
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to copy species \"%s\"", species_s);
				}


			if (copied_accession_s)
				{
					FreeCopiedString (copied_accession_s);
				}

		}	/* if ((IsStringEmpty (accession_s)) || ((copied_accession_s = EasyCopyToNewString (accession_s)) != NULL)) */
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to copy accession \"%s\"", accession_s);
		}





	return NULL;
}


Material *AllocateMaterialByGermplasmID (bson_oid_t *id_p, const char *germplasm_id_s, const Study *area_p, const FieldTrialServiceData *data_p)
{
	char *copied_germplasm_id_s = EasyCopyToNewString (germplasm_id_s);

	if (copied_germplasm_id_s)
		{
			Material *material_p = (Material *) AllocMemory (sizeof (Material));

			if (material_p)
				{
					memset (material_p, 0, sizeof (Material));

					material_p -> ma_id_p = id_p;
					material_p -> ma_parent_area_p = area_p;
					material_p -> ma_germplasm_id_s = copied_germplasm_id_s;



					return material_p;
				}		/* if (material_p) */

			FreeCopiedString (copied_germplasm_id_s);
		}

	return NULL;
}




Material *GetOrCreateMaterialByInternalName (const char *material_s, Study *area_p, const FieldTrialServiceData *data_p)
{
	Material *material_p = GetMaterialByGermplasmID (material_s, area_p, data_p);

	if (!material_p)
		{
			material_p  = AllocateMaterialByGermplasmID (NULL, material_s, area_p, data_p);

			if (!material_p)
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate Material with internal name of \"%s\" for area \"%s\"", material_s, area_p -> st_name_s);
				}

		}		/* if (!material_p) */

	return material_p;
}



json_t *GetMaterialAsJSON (const Material *material_p, const ViewFormat format, const FieldTrialServiceData *data_p)
{
	json_t *material_json_p = json_object ();

	if (material_json_p)
		{
			if (AddCompoundIdToJSON (material_json_p, material_p -> ma_id_p))
				{
					bool success_flag = false;

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
						}

					if (success_flag)
						{
							if (format == VF_STORAGE)
								{
									if (!AddNamedCompoundIdToJSON (material_json_p, material_p -> ma_parent_area_p -> st_id_p, MA_EXPERIMENTAL_AREA_ID_S))
										{
											char id_s [MONGO_OID_STRING_BUFFER_SIZE];

											bson_oid_to_string (material_p -> ma_parent_area_p -> st_id_p, id_s);
											PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, material_json_p, "Failed to add compound id for \"%s\": \"%s\"", MA_EXPERIMENTAL_AREA_ID_S, id_s);

											success_flag = false;
										}
								}

							if (success_flag)
								{
									success_flag = false;

									if (SetJSONString (material_json_p, MA_ACCESSION_S, material_p -> ma_accession_s))
										{
											if ((material_p -> ma_accession_full_name_s == NULL) || (SetJSONString (material_json_p, MA_ACCESSION_FULL_S, material_p -> ma_accession_full_name_s)))
												{
													success_flag = true;
												}		/* if ((material_p -> ma_accession_full_name_s == NULL) || (SetJSONString (material_json_p, MA_ACCESSION_FULL_S, material_p -> ma_accession_full_name_s))) */
											else
												{
													PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, material_json_p, "Failed to add \"%s\": \"%s\"", MA_ACCESSION_FULL_S, material_p -> ma_accession_full_name_s);

												}
											/*
											if (SetValidJSONString (material_json_p, MA_GERMPLASM_ORIGIN_S, material_p -> ma_germplasm_origin_s))
												{
													if (SetValidJSONString (material_json_p, MA_SEED_SOURCE_S, material_p -> ma_seed_source_s))
														{
															if (SetValidJSONString (material_json_p, MA_SEED_SUPPLIER_S, material_p -> ma_seed_supplier_s))
																{
																	if (SetValidJSONString (material_json_p, MA_SELECTION_REASON_S, material_p -> ma_selection_reason_s))
																		{
																			if (SetValidJSONString (material_json_p, MA_SEED_TREATMENT_S, material_p -> ma_seed_treatment_s))
																				{
																					if (SetValidJSONString (material_json_p, MA_GENERATION_S, material_p -> ma_generation_s))
																						{
																							if (SetValidJSONString (material_json_p, MA_SPECIES_S, material_p -> ma_species_name_s))
																								{
																									if (SetValidJSONString (material_json_p, MA_TYPE_S, material_p -> ma_type_s))
																										{
																											if (SetJSONBoolean (material_json_p, MA_IN_GRU_S, material_p -> ma_in_gru_flag))
																												{
																													if (SetJSONBoolean (material_json_p, MA_CLEANED_NAME_S, material_p -> ma_cleaned_flag))
																														{
																															if (SetJSONInteger (material_json_p, MA_TGW_S, material_p -> ma_tgw))
																																{
											 */
											/*
																																}
																															else
																																{
																																	PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, material_json_p, "Failed to add \"%s\": " UINT32_FMT, MA_TGW_S, material_p -> ma_tgw);
																																}

																														}
																													else
																														{
																															PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, material_json_p, "Failed to add \"%s\": \"%s\"", MA_CLEANED_NAME_S, material_p -> ma_cleaned_flag ? "true" : "false");
																														}
																												}
																											else
																												{
																													PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, material_json_p, "Failed to add \"%s\": \"%s\"", MA_IN_GRU_S, material_p -> ma_in_gru_flag ? "true" : "false");

																												}
																										}
																									else
																										{
																											PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, material_json_p, "Failed to add \"%s\": \"%s\"", MA_TYPE_S, material_p -> ma_type_s);
																										}

																								}
																							else
																								{
																									PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, material_json_p, "Failed to add \"%s\": \"%s\"", MA_SPECIES_S, material_p -> ma_species_name_s);
																								}
																						}
																					else
																						{
																							PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, material_json_p, "Failed to add \"%s\": \"%s\"", MA_GENERATION_S, material_p -> ma_generation_s);
																						}

																				}
																			else
																				{
																					PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, material_json_p, "Failed to add \"%s\": \"%s\"", MA_SEED_TREATMENT_S, material_p -> ma_seed_treatment_s);
																				}
																		}
																	else
																		{
																			PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, material_json_p, "Failed to add \"%s\": \"%s\"", MA_SELECTION_REASON_S, material_p -> ma_selection_reason_s);
																		}

																}
															else
																{
																	PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, material_json_p, "Failed to add \"%s\": \"%s\"", MA_SEED_SUPPLIER_S, material_p -> ma_seed_supplier_s);
																}
														}
													else
														{
															PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, material_json_p, "Failed to add \"%s\": \"%s\"", MA_SEED_SOURCE_S, material_p -> ma_seed_source_s);
														}
												}
											else
												{
													PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, material_json_p, "Failed to add \"%s\": \"%s\"", MA_GERMPLASM_ORIGIN_S, material_p -> ma_germplasm_origin_s);
												}

											if (AddDatatype (material_json_p, DFTD_MATERIAL))
												{
													return material_json_p;
												}
											 */

										}		/* if (SetJSONString (material_json_p, MA_ACCESSION_S, material_p -> ma_accession_s) */

									else
										{
											PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, material_json_p, "Failed to add \"%s\": \"%s\"", MA_ACCESSION_S, material_p -> ma_accession_s);
										}

								}		/* if (success_flag) */

						}		/* if (success_flag)) */
					else
						{
							char id_s [MONGO_OID_STRING_BUFFER_SIZE];

							bson_oid_to_string (material_p -> ma_gene_bank_id_p, id_s);
							PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, material_json_p, "Failed to add gene with id for \"%s\": \"%s\", expanded = %d", MA_GENE_BANK_ID_S, id_s, format);
						}
				}		/* if (AddCompoundIdToJSON (material_json_p, material_p -> ma_id_p)) */
			else
				{
					char id_s [MONGO_OID_STRING_BUFFER_SIZE];

					bson_oid_to_string (material_p -> ma_id_p, id_s);
					PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, material_json_p, "Failed to add compound id for \"%s\": \"%s\"", MONGO_ID_S, id_s);
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
			const char *germplasm_id_s = GetJSONString (json_p, MA_GERMPLASM_ID_S);

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
											Study *study_p = NULL;
											bool success_flag = false;

											if (format == VF_CLIENT_FULL)
												{
													bson_oid_t *study_id_p = GetNewUnitialisedBSONOid ();

													if (study_id_p)
														{
															if (GetNamedIdFromJSON (json_p, MA_EXPERIMENTAL_AREA_ID_S, study_id_p))
																{
																	study_p = GetStudyById (study_id_p, format, data_p);

																	if (!study_p)
																		{
																			char id_s [MONGO_OID_STRING_BUFFER_SIZE];

																			bson_oid_to_string (study_id_p, id_s);
																			PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, json_p, "Failed to find Study with id \"%s\"", id_s);
																		}
																}
															else
																{
																	PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, json_p, "Failed to get \"%s\"", MA_EXPERIMENTAL_AREA_ID_S);
																}

															FreeBSONOid (study_id_p);
														}		/* if (exp_area_id_p) */
													else
														{
															PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate id for \"%s\"", MA_EXPERIMENTAL_AREA_ID_S);
														}
												}
											else
												{
													success_flag = true;
												}

											if (success_flag)
												{
													Material *material_p = NULL;
													const char *type_s = GetJSONString (json_p, MA_TYPE_S);
													const char *species_s = GetJSONString (json_p, MA_SPECIES_S);
													const char *selection_reason_s = GetJSONString (json_p, MA_SELECTION_REASON_S);
													const char *generation_s = GetJSONString (json_p, MA_GENERATION_S);
													const char *seed_supplier_s = GetJSONString (json_p, MA_SEED_SUPPLIER_S);
													const char *seed_source_s = GetJSONString (json_p, MA_SEED_SOURCE_S);
													const char *germplasm_origin_s = GetJSONString (json_p, MA_GERMPLASM_ORIGIN_S);
													const char *seed_treatment_s = GetJSONString (json_p, MA_SEED_TREATMENT_S);
													const char *full_accession_s = GetJSONString (json_p, MA_ACCESSION_FULL_S);

													bool in_gru_flag;
													bool cleaned_flag;
													uint32 tgw = 0;


													GetJSONBoolean (json_p, MA_IN_GRU_S, &in_gru_flag);
													GetJSONBoolean (json_p, MA_CLEANED_NAME_S, &cleaned_flag);
													GetJSONUnsignedInteger (json_p, MA_TGW_S, (int *) &tgw);

													material_p = AllocateMaterial (id_p, accession_s, full_accession_s, species_s, type_s, selection_reason_s, generation_s, seed_supplier_s, seed_source_s, germplasm_origin_s, seed_treatment_s, in_gru_flag, cleaned_flag, tgw, study_p, gene_bank_id_p, data_p);


													if (material_p)
														{
															return material_p;
														}
													else
														{
															PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, json_p, "Failed to allocate Material");
														}
												}

											if (study_p)
												{
													FreeStudy (study_p);
												}

										}		/* if (GetNamedIdFromJSON (json_p, MA_GENE_BANK_ID_S, germplasm_id_p)) */
									else
										{
											PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, json_p, "Failed to get \"%s\"", MA_GENE_BANK_ID_S);
										}

									FreeBSONOid (gene_bank_id_p);
								}		/* if (germplasm_id_p) */
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
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed tp allocate id for \"%s\"", MONGO_ID_S);
				}

		}		/* if (accession_s) */
	else
		{
			PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, json_p, "Failed to get \"%s\"", MA_ACCESSION_S);
		}

	return NULL;
}


Material *GetMaterialByGermplasmID (const char *material_s, Study *area_p, const FieldTrialServiceData *data_p)
{
	Material *material_p = NULL;
	bson_t *query_p = BCON_NEW (MA_GERMPLASM_ID_S, BCON_UTF8 (material_s), MA_EXPERIMENTAL_AREA_ID_S, BCON_OID (area_p -> st_id_p));

	if (query_p)
		{
			material_p = SearchForMaterial (query_p, data_p);

			if (material_p)
				{
					material_p -> ma_parent_area_p = area_p;
				}

			bson_destroy (query_p);
		}		/* if (query_p) */

	return material_p;
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

																									if (GetJSONUnsignedInteger (table_row_json_p, S_TGW_TITLE_S, (int *) &tgw))
																										{
																											const char *treatment_s = GetJSONString (table_row_json_p, S_SEED_TREATMENT_TITLE_S);

																											if (!IsStringEmpty (treatment_s))
																												{
																													bool cleaned_flag;

																													if (GetJSONBoolean (table_row_json_p, S_CLEANED_TITLE_S, &cleaned_flag))
																														{
																															Material *material_p = AllocateMaterial (NULL, accession_s, species_name_s, type_s, reason_s, generation_s, supplier_s, source_s, germplasm_origin_s, treatment_s, in_gru_flag, cleaned_flag, tgw, area_p, gene_bank_p -> gb_id_p, data_p);

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
