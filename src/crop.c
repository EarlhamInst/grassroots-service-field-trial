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
 * crop.c
 *
 *  Created on: 12 Apr 2019
 *      Author: billy
 */

#define ALLOCATE_CROP_TAGS (1)
#include "crop.h"
#include "dfw_util.h"

#include "memory_allocations.h"
#include "string_utils.h"
#include "mongodb_util.h"


static void *GetCropCallback (const json_t *json_p, const ViewFormat format, const FieldTrialServiceData *data_p);


Crop *AllocateCrop (bson_oid_t *id_p, const char *name_s, const char *argovoc_preferred_term_s, const char *agrovoc_uri_s, char **synonyms_ss)
{
	char *copied_name_s = EasyCopyToNewString (name_s);

	if (copied_name_s)
		{
			char *copied_argovoc_preferred_term_s = EasyCopyToNewString (argovoc_preferred_term_s);

			if (copied_argovoc_preferred_term_s)
				{
					char *copied_agrovoc_uri_s = EasyCopyToNewString (agrovoc_uri_s);

					if (copied_agrovoc_uri_s)
						{
							char **copied_synonyms_ss = NULL;
							bool success_flag = true;

							if (synonyms_ss)
								{
									size_t num_synonyms = 0;
									char **synonym_ss = synonyms_ss;

									while (*synonym_ss)
										{
											++ num_synonyms;
											++ synonym_ss;
										}

									if (num_synonyms > 0)
										{
											copied_synonyms_ss = (char **) AllocMemoryArray (num_synonyms + 1, sizeof (char *));

											if (copied_synonyms_ss)
												{
													char **copied_synonym_ss = copied_synonyms_ss;

													synonym_ss = synonyms_ss;

													/*
													 * Copy the synonyms
													 */
													while ((*synonym_ss) && success_flag)
														{
															*copied_synonym_ss = EasyCopyToNewString (*synonym_ss);

															if (*copied_synonym_ss)
																{
																	++ synonym_ss;
																	++ copied_synonym_ss;
																}
															else
																{
																	success_flag = false;
																}
														}

													if (!success_flag)
														{
															/*
															 * The last synonym copy failed, so go back to the previous one
															 */
															-- copied_synonym_ss;

															while (copied_synonym_ss >= copied_synonyms_ss)
																{
																	FreeCopiedString (*copied_synonym_ss);
																	-- copied_synonym_ss;
																}

															FreeMemory (copied_synonyms_ss);
														}
												}

										}		/* if (num_synonyms > 0) */

								}		/* if (synonyms_ss) */

							if (success_flag)
								{
									Crop *crop_p = (Crop *) AllocMemory (sizeof (Crop));

									if (crop_p)
										{
											crop_p -> cr_id_p = id_p;
											crop_p -> cr_name_s = copied_name_s;
											crop_p -> cr_argovoc_preferred_term_s = copied_argovoc_preferred_term_s;
											crop_p -> cr_agrovoc_uri_s = copied_agrovoc_uri_s;
											crop_p -> cr_synonyms_ss = copied_synonyms_ss;

											return crop_p;
										}		/* if (crop_p) */

								}		/* if (success_flag) */


							FreeCopiedString (copied_agrovoc_uri_s);
						}		/* if (copied_agrovoc_uri_s) */

					FreeCopiedString (copied_argovoc_preferred_term_s);
				}		/* if (copied_argovoc_preferred_term_s) */

			FreeCopiedString (copied_name_s);
		}		/* if (copied_name_s) */

	return NULL;

}



void FreeCrop (Crop *crop_p)
{
	if (crop_p -> cr_id_p)
		{
			FreeBSONOid (crop_p -> cr_id_p);
		}

	if (crop_p -> cr_argovoc_preferred_term_s)
		{
			FreeCopiedString (crop_p -> cr_argovoc_preferred_term_s);
		}

	if (crop_p -> cr_agrovoc_uri_s)
		{
			FreeCopiedString (crop_p -> cr_agrovoc_uri_s);
		}

	FreeCopiedString (crop_p -> cr_name_s);

	if (crop_p -> cr_synonyms_ss)
		{
			char **synonym_ss = crop_p -> cr_synonyms_ss;

			while (*synonym_ss)
				{
					FreeCopiedString (*synonym_ss);
					++ synonym_ss;
				}

			FreeMemory (crop_p -> cr_synonyms_ss);
		}

	FreeMemory (crop_p);
}


Crop *CopyCrop (const Crop * const src_p)
{
	bson_oid_t *copied_id_p = CopyBSONOid (src_p -> cr_id_p);

	if (copied_id_p)
		{
			Crop *dest_p = AllocateCrop (copied_id_p, src_p -> cr_name_s, src_p -> cr_argovoc_preferred_term_s, src_p -> cr_agrovoc_uri_s, src_p -> cr_synonyms_ss);

			if (dest_p)
				{
					return dest_p;
				}

			FreeBSONOid (copied_id_p);
		}


	return NULL;
}


json_t *GetCropAsJSON (Crop *crop_p, const ViewFormat format, const FieldTrialServiceData *data_p)
{
	json_t *res_p = json_object ();

	if (res_p)
		{
			if (SetJSONString (res_p, CR_NAME_S, crop_p -> cr_name_s))
				{
					if (SetJSONString (res_p, CR_AGROVOC_URL_S, crop_p -> cr_agrovoc_uri_s))
						{
							if (SetJSONString (res_p, CR_AGROVOC_PREFERRED_TERM_S, crop_p -> cr_argovoc_preferred_term_s))
								{
									bool success_flag = true;

									if (crop_p -> cr_synonyms_ss)
										{
											json_t *synonyms_json_p = json_array ();

											if (synonyms_json_p)
												{
													char **synonym_ss = crop_p -> cr_synonyms_ss;

													while (success_flag && (*synonym_ss))
														{
															json_t *synonym_p = json_string (*synonym_ss);

															if (synonym_p)
																{
																	if (json_array_append_new (synonyms_json_p, synonym_p) == 0)
																		{
																			++ synonym_ss;
																		}
																	else
																		{
																			PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, synonyms_json_p, "Failed to add synonym for \"%s\"", *synonym_ss);
																			success_flag = false;
																		}
																}
															else
																{
																	PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate JSON synonym for \"%s\"", *synonym_ss);
																	success_flag = false;
																}

														}		/* while (success_flag && (*synonym_ss)) */

													if (success_flag)
														{
															if (json_object_set_new (res_p, CR_SYNONYMS_S, synonyms_json_p) != 0)
																{
																	success_flag = false;
																}
														}

													if (!success_flag)
														{
															json_decref (synonyms_json_p);
														}

												}		/* if (synonyms_json_p) */

										}		/* if (crop_p -> cr_synonyms_ss) */

									if (success_flag)
										{
											if (AddCompoundIdToJSON (res_p, crop_p -> cr_id_p))
												{
													if (AddDatatype (res_p, DFTD_CROP))
														{
															return res_p;
														}

												}		/* if (AddCompoundIdToJSON (res_p, gene_bank_p -> gb_id_p)) */
											else
												{
													char id_s [MONGO_OID_STRING_BUFFER_SIZE];

													bson_oid_to_string (crop_p -> cr_id_p, id_s);
													PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, res_p, "Failed to add compound id for \"%s\": \"%s\"", MONGO_ID_S, id_s);
												}

										}
								}		/* if (SetJSONString (res_p, CR_AGROVOC_PREFERRED_TERM_S, crop_p -> cr_argovoc_preferred_term_s)) */
							else
								{
									PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, res_p, "Failed to add \"%s\": \"%s\"", CR_AGROVOC_PREFERRED_TERM_S, crop_p -> cr_argovoc_preferred_term_s);
								}

						}		/* if (SetJSONString (res_p, CR_AGROVOC_URL_S, crop_p -> cr_agrovoc_uri_s)) */
					else
						{
							PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, res_p, "Failed to add \"%s\": \"%s\"", CR_AGROVOC_URL_S, crop_p -> cr_agrovoc_uri_s);
						}

				}		/* if (SetJSONString (res_p, CR_NAME_S, crop_p -> cr_name_s)) */
			else
				{
					PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, res_p, "Failed to add \"%s\": \"%s\"", CR_NAME_S,  crop_p -> cr_name_s);
				}

			json_decref (res_p);
		}

	return NULL;
}



Crop *GetCropFromJSON (const json_t *crop_json_p, const FieldTrialServiceData *data_p)
{
	const char *name_s = GetJSONString (crop_json_p, CR_NAME_S);

	if (name_s)
		{
			const char *url_s = GetJSONString (crop_json_p, CR_AGROVOC_URL_S);

			if (url_s)
				{
					const char *term_s = GetJSONString (crop_json_p, CR_AGROVOC_PREFERRED_TERM_S);

					if (term_s)
						{
							bool success_flag = true;
							char **synonyms_ss = NULL;
							const json_t *synonyms_p = json_object_get (crop_json_p, CR_SYNONYMS_S);

							if (synonyms_p)
								{
									const size_t num_synonyms = json_array_size (synonyms_p);

									synonyms_ss = (char **) AllocMemoryArray (num_synonyms + 1, sizeof (char *));

									if (synonyms_ss)
										{
											char **synonym_ss = synonyms_ss;
											size_t i = 0;

											/*
											 * Copy the synonyms
											 */
											while ((i < num_synonyms) && (success_flag))
												{
													const json_t *synonym_p = json_array_get (synonyms_p, i);
													const char *value_s = json_string_value (synonym_p);

													if (value_s)
														{
															*synonym_ss = EasyCopyToNewString (value_s);

															if (*synonym_ss)
																{
																	++ i;
																	++ synonym_ss;
																}
															else
																{
																	PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to copy synonym \"%s\"", value_s);
																	success_flag = false;
																}
														}
													else
														{
															PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, synonym_p, "synonym is not a string");
															success_flag = false;
														}

												}		/* while ((i < num_synonyms) && (success_flag)) */

										}		/* if (synonyms_ss) */
									else
										{
											PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, crop_json_p, "Failed to allocate memory for synonyms");
											success_flag = false;
										}

								}		/* if (synonyms_p) */

							if (success_flag)
								{
									bson_oid_t *id_p = GetNewUnitialisedBSONOid ();

									if (id_p)
										{
											if (GetMongoIdFromJSON (crop_json_p, id_p))
												{
													Crop *crop_p = AllocateCrop (id_p, name_s, term_s, url_s, synonyms_ss);

													if (synonyms_ss)
														{
															char **synonym_ss = synonyms_ss;

															while (*synonym_ss)
																{
																	FreeCopiedString (*synonym_ss);
																	++ synonym_ss;
																}

															FreeMemory (synonyms_ss);
														}

													if (crop_p)
														{
															return crop_p;
														}		/* if (crop_p) */
													else
														{
															PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, crop_json_p, "Failed to allocate Crop");
														}

												}		/* if (GetMongoIdFromJSON (gene_bank_json_p, id_p)) */
											else
												{
													PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, crop_json_p, "Failed to get \"%s\"", MONGO_ID_S);
												}

											FreeBSONOid (id_p);
										}		/* if (id_p) */
									else
										{
											PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, crop_json_p, "Failed to allocate uninitialised id");
										}

								}		/* if (success_flag) */

						}		/* if (term_s) */
					else
						{
							PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, crop_json_p, "Failed to get \"%s\"", CR_AGROVOC_PREFERRED_TERM_S);
						}

				}		/* if (url_s) */
			else
				{
					PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, crop_json_p, "Failed to get \"%s\"", CR_AGROVOC_URL_S);
				}

		}		/* if (name_s) */
	else
		{
			PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, crop_json_p, "Failed to get \"%s\"", CR_NAME_S);
		}

	return NULL;

}


bool SaveCrop (Crop *crop_p, const FieldTrialServiceData *data_p)
{
	bson_t *selector_p = NULL;
	bool success_flag = PrepareSaveData (& (crop_p -> cr_id_p), &selector_p);

	if (crop_p -> cr_id_p)
		{
			json_t *crop_json_p = GetCropAsJSON (crop_p, VF_STORAGE, data_p);

			if (crop_json_p)
				{
					MongoTool *mongo_p = GetConfiguredMongoTool (data_p, NULL);

					if (mongo_p)
						{
							success_flag = SaveAndBackupMongoDataWithTimestamp (mongo_p, crop_json_p, data_p -> dftsd_collection_ss [DFTD_CROP], data_p -> dftsd_backup_collection_ss [DFTD_CROP], DFT_BACKUPS_ID_KEY_S, selector_p, MONGO_TIMESTAMP_S);

							FreeMongoTool (mongo_p);
						}
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "GetConfiguredMongoTool () failed");
						}

					json_decref (crop_json_p);
				}		/* if (crop_json_p) */

		}		/* if (crop_p -> cr_id_p) */

	return success_flag;
}


Crop *GetCropById (const bson_oid_t *id_p, const FieldTrialServiceData *data_p)
{
	Crop *crop_p = NULL;
	char *id_s = GetBSONOidAsString (id_p);

	if (id_s)
		{
			crop_p = GetCropByIdString (id_s, data_p);
			FreeBSONOidString (id_s);
		}
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "GetBSONOidAsString failed for crop");
		}

	return crop_p;
}



Crop *GetCropByIdString (const char *id_s, const FieldTrialServiceData *data_p)
{
	Crop *crop_p = (Crop *) GetDFWObjectByIdString (id_s, DFTD_CROP, GetCropCallback, VF_STORAGE, data_p);

	return crop_p;
}


static void *GetCropCallback (const json_t *json_p, const ViewFormat format, const FieldTrialServiceData *data_p)
{
	return GetCropFromJSON (json_p, data_p);
}
