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
 * program.c
 *
 *  Created on: 7 Jan 2019
 *      Author: billy
 */


#define ALLOCATE_PROGRAMME_TAGS (1)
#include "programme.h"

#include "memory_allocations.h"
#include "dfw_util.h"
#include "indexing.h"

#include "programme_jobs.h"
#include "mongodb_util.h"





static LinkedList *GetMatchingProgrammes (const FieldTrialServiceData *data_p, const char **keys_ss, const char **values_ss);

static void *GetProgrammeObjectFromJSON (const json_t *json_p, const ViewFormat format, const FieldTrialServiceData *data_p);

static bool AddFullDetailsToProgrammeJSON (const Programme *programme_p, json_t *programme_json_p, const bool add_id_flag);

static bool AddPIToJSON (json_t *programme_json_p, const Person *person_p, const ViewFormat format, const FieldTrialServiceData *data_p);

static bool AddFundingToClientProgrammeJSON (const Programme *programme_p, json_t *programme_json_p);




Programme *AllocateProgramme (bson_oid_t *id_p, Metadata *metadata_p, const char *abbreviation_s, Crop *crop_p, const char *documentation_url_s, const char *name_s, const char *objective_s, Person *pi_p, const char *logo_url_s, const char *funders_s, const char *project_code_s)
{
	char *copied_abbreviation_s = NULL;

	if ((abbreviation_s == NULL) || (copied_abbreviation_s = EasyCopyToNewString (abbreviation_s)))
		{
			char *copied_documentation_url_s = NULL;

			if ((documentation_url_s == NULL) || (copied_documentation_url_s = EasyCopyToNewString (documentation_url_s)))
				{
					char *copied_name_s = EasyCopyToNewString (name_s);

					if (copied_name_s)
						{
							char *copied_objective_s = NULL;

							if ((objective_s == NULL) || (copied_objective_s = EasyCopyToNewString (objective_s)))
								{
									char *copied_logo_url_s = NULL;

									if ((logo_url_s == NULL) || (copied_logo_url_s = EasyCopyToNewString (logo_url_s)))
										{
											char *copied_funders_s = NULL;

											if ((funders_s == NULL) || (copied_funders_s = EasyCopyToNewString (funders_s)))
												{
													char *copied_project_code_s = NULL;

													if ((project_code_s == NULL) || (copied_project_code_s = EasyCopyToNewString (project_code_s)))
														{

															LinkedList *trials_p = AllocateLinkedList (FreeFieldTrialNode);

															if (trials_p)
																{

																	if (pi_p)
																		{

																			if (!metadata_p)
																				{

																				}

//																			if (metadata_p)
//																				{

																					Programme *programme_p = (Programme *) AllocMemory (sizeof (Programme));

																					if (programme_p)
																						{
																							programme_p -> pr_abbreviation_s = copied_abbreviation_s;
																							programme_p -> pr_crop_p = crop_p;
																							programme_p -> pr_documentation_url_s = copied_documentation_url_s;
																							programme_p -> pr_id_p = id_p;
																							programme_p -> pr_metadata_p = metadata_p;
																							programme_p -> pr_name_s = copied_name_s;
																							programme_p -> pr_objective_s = copied_objective_s;
																							programme_p -> pr_pi_p = pi_p;
																							programme_p -> pr_trials_p = trials_p;
																							programme_p -> pr_logo_url_s = copied_logo_url_s;
																							programme_p -> pr_project_code_s = copied_project_code_s;
																							programme_p -> pr_funding_organisation_s = copied_funders_s;
																							programme_p -> pr_pi_p = pi_p;

																							return programme_p;
																						}		/* if (programme_p) */

//																				}

																		}




																	FreeLinkedList (trials_p);
																}		/* if (trials_p) */




															if (copied_project_code_s)
																{
																	FreeCopiedString (copied_project_code_s);
																}

														}		/* if ((project_code_s == NULL) || (copied_project_code_s = EasyCopyToNewString (project_code_s))) */

													if (copied_funders_s)
														{
															FreeCopiedString (copied_funders_s);
														}

												}		/* if ((funders_s == NULL) || (copied_funders_s = EasyCopyToNewString (funders_s))) */

											if (copied_logo_url_s)
												{
													FreeCopiedString (copied_logo_url_s);
												}
										}

									if (copied_objective_s)
										{
											FreeCopiedString (copied_objective_s);
										}

								}		/* if ((objective_s == NULL) || (copied_objective_s = EasyCopyToNewString (objective_s))) */

							FreeCopiedString (copied_name_s);
						}		/* if (copied_name_s) */


					if (copied_documentation_url_s)
						{
							FreeCopiedString (copied_documentation_url_s);
						}

				}		/* if ((documentation_url_s == NULL) || (copied_documentation_url_s = EasyCopyToNewString (documentation_url_s))) */


			if (copied_abbreviation_s)
				{
					FreeCopiedString (copied_abbreviation_s);
				}

		}		/* if ((abbreviation_s == NULL) || (copied_abbreviation_s = EasyCopyToNewString (abbreviation_s))) */

	return NULL;
}


void FreeProgramme (Programme *programme_p)
{
	if (programme_p -> pr_id_p)
		{
			FreeBSONOid (programme_p -> pr_id_p);
		}


	if (programme_p -> pr_metadata_p)
		{
			FreeMetadata (programme_p -> pr_metadata_p);
		}

	if (programme_p -> pr_abbreviation_s)
		{
			FreeCopiedString (programme_p -> pr_abbreviation_s);
		}

	if (programme_p -> pr_crop_p)
		{
			FreeCrop (programme_p -> pr_crop_p);
		}

	if (programme_p -> pr_documentation_url_s)
		{
			FreeCopiedString (programme_p -> pr_documentation_url_s);
		}

	FreeCopiedString (programme_p -> pr_name_s);

	if (programme_p -> pr_objective_s)
		{
			FreeCopiedString (programme_p -> pr_objective_s);
		}

	if (programme_p -> pr_pi_p)
		{
			FreePerson (programme_p -> pr_pi_p);
		}

	FreeLinkedList (programme_p -> pr_trials_p);

	if (programme_p -> pr_logo_url_s)
		{
			FreeCopiedString (programme_p -> pr_logo_url_s);
		}

	if (programme_p -> pr_funding_organisation_s)
		{
			FreeCopiedString (programme_p -> pr_funding_organisation_s);
		}

	if (programme_p -> pr_project_code_s)
		{
			FreeCopiedString (programme_p -> pr_project_code_s);
		}


	FreeMemory (programme_p);
}


ProgrammeNode *AllocateProgrammeNode (Programme *programme_p)
{
	ProgrammeNode *node_p = (ProgrammeNode *) AllocMemory (sizeof (ProgrammeNode));

	if (node_p)
		{
			InitListItem (& (node_p -> pn_node));

			node_p -> pn_programme_p = programme_p;
		}

	return node_p;
}


void FreeProgrammeNode (ListItem *node_p)
{
	ProgrammeNode *pr_node_p = (ProgrammeNode *) node_p;

	FreeProgramme (pr_node_p -> pn_programme_p);

	FreeMemory (pr_node_p);
}



json_t *GetProgrammeAsJSON (Programme *programme_p, const ViewFormat format, const FieldTrialServiceData *data_p)
{
	json_t *programme_json_p = json_object ();

	if (programme_json_p)
		{
			if (SetNonTrivialString (programme_json_p, PR_NAME_S, programme_p -> pr_name_s, true))
				{
					if (AddPIToJSON (programme_json_p, programme_p -> pr_pi_p, format, data_p))
						{
							if (SetNonTrivialString (programme_json_p, PR_LOGO_S, programme_p -> pr_logo_url_s, true))
								{
									if (AddDatatype (programme_json_p, DFTD_PROGRAMME))
										{
											if (AddFullDetailsToProgrammeJSON (programme_p, programme_json_p, true))
												{
													if (AddMetadataToJSON (programme_p -> pr_metadata_p, programme_json_p, format))
														{
															bool success_flag = false;

															switch (format)
															{
																case VF_CLIENT_FULL:
																case VF_CLIENT_MINIMAL:
																	{

																		if (AddFundingToClientProgrammeJSON (programme_p, programme_json_p))
																			{
																				if (programme_p -> pr_crop_p)
																					{
																						if (SetJSONString (programme_json_p, PR_CROP_S, programme_p -> pr_crop_p -> cr_name_s))
																							{
																								success_flag = true;
																							}
																						else
																							{
																								PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, programme_json_p, "Failed to add crop \"%s\"", programme_p -> pr_crop_p -> cr_name_s);
																							}
																					}		/* if (programme_p -> pr_crop_id_p) */
																				else
																					{
																						success_flag = true;
																					}

																			}
																	}
																	break;


																case VF_INDEXING:
																	{
																		if ((programme_p -> pr_funding_organisation_s == NULL) || (SetJSONString (programme_json_p, PR_FUNDERS_S, programme_p -> pr_funding_organisation_s)))
																			{
																				if ((programme_p -> pr_project_code_s == NULL) || (SetJSONString (programme_json_p, PR_CODE_S, programme_p -> pr_project_code_s)))
																					{
																						if (programme_p -> pr_crop_p)
																							{
																								if (SetJSONString (programme_json_p, PR_CROP_S, programme_p -> pr_crop_p -> cr_name_s))
																									{
																										success_flag = true;
																									}
																								else
																									{
																										PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, programme_json_p, "Failed to add crop \"%s\"", programme_p -> pr_crop_p -> cr_name_s);
																									}
																							}		/* if (programme_p -> pr_crop_id_p) */
																						else
																							{
																								success_flag = true;
																							}
																					}
																			}
																	}
																	break;


																case VF_STORAGE:
																	{
																		if ((programme_p -> pr_funding_organisation_s == NULL) || (SetJSONString (programme_json_p, PR_FUNDERS_S, programme_p -> pr_funding_organisation_s)))
																			{
																				if ((programme_p -> pr_project_code_s == NULL) || (SetJSONString (programme_json_p, PR_CODE_S, programme_p -> pr_project_code_s)))
																					{
																						if ((! (programme_p -> pr_crop_p)) || (AddNamedCompoundIdToJSON (programme_json_p, programme_p -> pr_crop_p -> cr_id_p, PR_CROP_S)))
																							{
																								success_flag = true;
																							}
																						else
																							{
																								PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, programme_json_p, "Failed to add crop \"%s\" to json", programme_p -> pr_crop_p -> cr_name_s);
																							}
																					}
																				else
																					{
																						PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, programme_json_p, "SetJSONString () failed for \"%s\": \"%s\"", PR_CODE_S, programme_p -> pr_project_code_s);
																					}
																			}
																		else
																			{
																				PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, programme_json_p, "SetJSONString () failed for \"%s\": \"%s\"", PR_FUNDERS_S, programme_p -> pr_funding_organisation_s);
																			}
																	}
																	break;

																default:
																	success_flag = true;
																	break;
															}


															if (success_flag)
																{
																	return programme_json_p;
																}

														}
													else
														{
															PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, programme_json_p, "AddPermissionsGroupToJSON () failed");
														}

												}		/* if (AddFullDetailsToProgrammeJSON (programme_p, programme_json_p, format == VF_STORAGE)) */

										}


								}		/* if (SetNonTrivialString (programme_json_p, PR_LOGO_S, programme_p -> pr_logo_url_s)) */

						}		/* if (AddPIToJSON (programme_json_p, programme_p -> pr_pi_p, format, data_p))) */

				}		/* if (json_object_set_new (trial_json_p, name_key_s, json_string (trial_p -> ft_name_s)) == 0) */

			json_decref (programme_json_p);
		}		/* if (programme_json_p) */

	return NULL;

}


Programme *GetProgrammeFromJSON (const json_t *json_p, const ViewFormat format, const FieldTrialServiceData *data_p)
{
	const char *name_s = GetJSONString (json_p, PR_NAME_S);

	if (name_s)
		{
			Person *pi_p = GetPersonFromCompoundJSON (json_p, PR_PI_S, format, data_p);

			if (pi_p)
				{
					bson_oid_t *id_p = GetNewUnitialisedBSONOid ();

					if (id_p)
						{
							if (GetMongoIdFromJSON (json_p, id_p))
								{
									Programme *programme_p = NULL;
									const char *objective_s = GetJSONString (json_p, PR_OBJECTIVE_S);
									const char *documentation_url_s = GetJSONString (json_p, PR_DOCUMENTATION_URL_S);
									const char *abbreviation_s = GetJSONString (json_p, PR_ABBREVIATION_S);
									const char *logo_s = GetJSONString (json_p, PR_LOGO_S);
									const char *funders_s = GetJSONString (json_p, PR_FUNDERS_S);
									const char *project_code_s = GetJSONString (json_p, PR_CODE_S);
									Crop *crop_p = NULL;
									bson_oid_t *temp_id_p = GetNewUnitialisedBSONOid ();
									Metadata *metadata_p = GetMetadataFromDefaultChildJSON (json_p, & (data_p -> dftsd_base_data));

									if (temp_id_p)
										{
											if (GetNamedIdFromJSON (json_p, PR_CROP_S, temp_id_p))
												{
													crop_p = GetCropById (temp_id_p, data_p);
												}

											FreeBSONOid (temp_id_p);
										}

									programme_p = AllocateProgramme (id_p, metadata_p, abbreviation_s, crop_p, documentation_url_s, name_s, objective_s, pi_p, logo_s, funders_s, project_code_s);

									if (programme_p)
										{
											return programme_p;
										}

									if (crop_p)
										{
											FreeCrop (crop_p);
										}

								}

							FreeBSONOid (id_p);
						}

					FreePerson (pi_p);
				}		/* if (pi_p) */

		}

	return NULL;
}


bool AddProgrammeFieldTrial (Programme *programme_p, FieldTrial *trial_p, MEM_FLAG mf)
{
	bool success_flag = false;
	FieldTrialNode *node_p = AllocateFieldTrialNode (trial_p);

	if (node_p)
		{
			trial_p -> ft_parent_p = programme_p;
			trial_p -> ft_parent_program_mem = mf;

			LinkedListAddTail (programme_p -> pr_trials_p, & (node_p -> ftn_node));
			success_flag  = true;
		}

	return success_flag;
}



uint32 GetNumberOfProgrammeFieldTrials (const Programme *programme_p)
{
	return (programme_p -> pr_trials_p ? programme_p -> pr_trials_p -> ll_size : 0);
}



bool AddFieldTrialsToProgrammeJSON (Programme *programme_p, json_t *programme_json_p, const ViewFormat format, FieldTrialServiceData *data_p)
{
	bool success_flag = true;

	if (programme_p -> pr_trials_p -> ll_size > 0)
		{
			json_t *trials_p = json_array ();

			if (trials_p)
				{
					FieldTrialNode *node_p = (FieldTrialNode *) (programme_p -> pr_trials_p -> ll_head_p);
					bool ok_flag = true;

					while (node_p && ok_flag)
						{
							json_t *trial_p = GetFieldTrialAsJSON (node_p -> ftn_field_trial_p, format, data_p);

							if (trial_p)
								{
									if (json_array_append_new (trials_p, trial_p) == 0)
										{
											node_p = (FieldTrialNode *) (node_p -> ftn_node.ln_next_p);
										}
									else
										{
											ok_flag = false;
											PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, trial_p, "Failed to add Field Trial json to array for program \"%s\" - \"%s\"", programme_p -> pr_name_s, (programme_p -> pr_pi_p) ? programme_p -> pr_pi_p -> pe_name_s : "NULL" );
										}
								}
							else
								{
									char buffer_s [MONGO_OID_STRING_BUFFER_SIZE];

									ok_flag = false;
									bson_oid_to_string (node_p -> ftn_field_trial_p -> ft_id_p, buffer_s);

									PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, trial_p, "Failed to get Trial json for area \"%s\"", buffer_s);
								}
						}		/* while (node_p && success_flag) */

					if (ok_flag)
						{
							if (json_object_set_new (programme_json_p, PR_TRIALS_S, trials_p) == 0)
								{
									success_flag = true;
								}
							else
								{
									char buffer_s [MONGO_OID_STRING_BUFFER_SIZE];

									ok_flag = false;

									bson_oid_to_string (programme_p -> pr_id_p, buffer_s);

									PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, trials_p, "Failed to add Trial json to Programme \"%s\"", buffer_s);

									json_decref (trials_p);
								}
						}

				}		/* if (exp_areas_p) */
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate Trials json object for Programme \"%s\" - \"%s\"", programme_p -> pr_name_s, (programme_p -> pr_pi_p) ? programme_p -> pr_pi_p -> pe_name_s : "NULL" );
				}

		}		/* if (trial_p -> ft_studies_p -> ll_size > 0) */
	else
		{
			/* nothing to add */
			success_flag = true;
		}

	return success_flag;
}


/*
 * The Programme could be the bson_oid or a name so check
 */

Programme *GetUniqueProgrammeBySearchString (const char *programme_s, const ViewFormat format, const FieldTrialServiceData *data_p)
{
	Programme *programme_p = NULL;

	if (bson_oid_is_valid (programme_s, strlen (programme_s)))
		{
			programme_p = GetProgrammeByIdString (programme_s, format, data_p);
		}

	if (!programme_p)
		{
			LinkedList *programs_p = GetProgrammesByName (programme_s, data_p);

			if (programs_p)
				{
					if (programs_p -> ll_size == 1)
						{
							ProgrammeNode *node_p = (ProgrammeNode *) (programs_p -> ll_head_p);

							/* Remove the program from the node */
							programme_p = node_p -> pn_programme_p;
							node_p -> pn_programme_p = NULL;
						}

					FreeLinkedList (programs_p);
				}
		}

	return programme_p;
}


LinkedList *GetProgrammesByName (const char * const programme_s, const FieldTrialServiceData *data_p)
{
	const char *keys_ss [] = { PR_NAME_S, NULL };
	const char *values_ss [] = { programme_s, NULL };

	return GetMatchingProgrammes (data_p, keys_ss, values_ss);
}


static LinkedList *GetMatchingProgrammes (const FieldTrialServiceData *data_p, const char **keys_ss, const char **values_ss)
{
	LinkedList *field_trials_list_p = AllocateLinkedList (FreeProgrammeNode);

	if (field_trials_list_p)
		{
			bson_t *query_p = bson_new ();

			if (query_p)
				{
					bool success_flag = true;

					if (keys_ss && values_ss)
						{
							const char **key_ss = keys_ss;
							const char **value_ss = values_ss;

							while (success_flag && (*key_ss != NULL) && (*value_ss != NULL))
								{
									if (BSON_APPEND_UTF8 (query_p, *key_ss, *value_ss))
										{
											++ key_ss;
											++ value_ss;
										}
									else
										{
											success_flag = false;
										}
								}

						}

					if (success_flag)
						{
							MongoTool *mongo_p = GetConfiguredMongoTool (data_p, NULL);

							if (mongo_p)
								{
									if (SetMongoToolCollection (mongo_p, data_p -> dftsd_collection_ss [DFTD_FIELD_TRIAL]))
										{
											json_t *results_p = GetAllMongoResultsAsJSON (mongo_p, query_p, NULL);

											if (results_p)
												{
													const size_t size = json_array_size (results_p);
													size_t i = 0;

													for (i = 0; i < size; ++ i)
														{
															json_t *result_p = json_array_get (results_p, i);
															FieldTrial *trial_p = GetFieldTrialFromJSON (result_p, VF_STORAGE, data_p);

															if (trial_p)
																{
																	FieldTrialNode *node_p = AllocateFieldTrialNode (trial_p);

																	if (node_p)
																		{
																			LinkedListAddTail (field_trials_list_p, & (node_p -> ftn_node));
																		}
																	else
																		{
																			PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, result_p, "Failed to create FieldTrialNode");
																		}

																}		/* if (trial_p) */
															else
																{
																	PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, result_p, "Failed to get FieldTrial from JSON");
																}

														}		/* for (i = 0; i < size; ++ i) */


													json_decref (results_p);
												}		/* if (results_p) */

										}		/* if (SetMongoToolCollection (data_p -> dftsd_mongo_p, data_p -> dftsd_collection_ss [DFTD_FIELD_TRIAL])) */
									else
										{
											PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to set mongo tool collection to \"%s\"", data_p -> dftsd_collection_ss [DFTD_FIELD_TRIAL]);
										}

									FreeMongoTool (mongo_p);
								}		/* if (mongo_p) */
							else
								{
									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "GetConfiguredMongoTool () failed");
								}





						}		/* if (success_flag) */
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add query");
						}

					bson_destroy (query_p);
				}		/* if (query_p) */


		}		/* if (field_trials_list_p) */

	return field_trials_list_p;
}





Programme *GetProgrammeById (const bson_oid_t *id_p, const ViewFormat format, const FieldTrialServiceData *data_p)
{
	Programme *programme_p = NULL;
	char *id_s = GetBSONOidAsString (id_p);

	if (id_s)
		{
			programme_p = GetProgrammeByIdString (id_s, format, data_p);

			FreeBSONOidString (id_s);
		}

	return programme_p;
}

static void *GetProgrammeObjectFromJSON (const json_t *json_p, const ViewFormat format, const FieldTrialServiceData *data_p)
{
	return (void *) GetProgrammeFromJSON (json_p, format, data_p);
}


Programme *GetProgrammeByIdString (const char *program_id_s, const ViewFormat format, const FieldTrialServiceData *data_p)
{
	Programme *programme_p = NULL;

	if (bson_oid_is_valid (program_id_s, strlen (program_id_s)))
		{
			bson_oid_t *id_p = GetBSONOidFromString (program_id_s);

			if (id_p)
				{
					void *obj_p = GetDFWObjectById (id_p, DFTD_PROGRAMME, GetProgrammeObjectFromJSON, format, data_p);
					programme_p = (Programme *) obj_p;

					FreeBSONOid (id_p);
				}
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to create BSON OID for \"%s\"", program_id_s);
				}

		}		/* if (bson_oid_is_valid (field_trial_id_s, strlen (field_trial_id_s))) */
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "\"%s\" is not a valid oid", program_id_s);
		}

	return programme_p;
}



OperationStatus SaveProgramme (Programme *programme_p, ServiceJob *job_p, FieldTrialServiceData *data_p)
{
	OperationStatus status = OS_FAILED;
	bson_t *selector_p = NULL;
	bool success_flag = PrepareSaveData (& (programme_p -> pr_id_p), &selector_p);

	if (success_flag)
		{
			json_t *programme_json_p = GetProgrammeAsJSON (programme_p, VF_STORAGE, data_p);

			if (programme_json_p)
				{
					MongoTool *mongo_p = GetConfiguredMongoTool (data_p, NULL);

					if (mongo_p)
						{
							if (SaveAndBackupMongoDataWithTimestamp (mongo_p, programme_json_p, data_p -> dftsd_collection_ss [DFTD_PROGRAMME],
																											 data_p -> dftsd_backup_collection_ss [DFTD_PROGRAMME], DFT_BACKUPS_ID_KEY_S, selector_p, MONGO_TIMESTAMP_S))
								{
									char *id_s = GetBSONOidAsString (programme_p -> pr_id_p);
									json_t *programme_indexing_p = GetProgrammeAsJSON (programme_p, VF_INDEXING, data_p);

									if (programme_indexing_p)
										{
											status = IndexData (job_p, programme_indexing_p, NULL);
											json_decref (programme_indexing_p);
										}

									if (status != OS_SUCCEEDED)
										{
											status = OS_PARTIALLY_SUCCEEDED;
											PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, programme_json_p, "Failed to index Programme \"%s\" as JSON to Lucene", programme_p -> pr_name_s);
											AddGeneralErrorMessageToServiceJob (job_p, "Programme saved but failed to index for searching");
										}

									if (data_p -> dftsd_assets_path_s)
										{
											if (!SaveProgrammeAsFrictionlessData (programme_p, data_p))
												{

												}
										}

									if (id_s)
										{
											/*
											 * If we have the front-end web address to view the trial,
											 * save it to the ServiceJob.
											 */
											if (data_p -> dftsd_view_programme_url_s)
												{
													SetFieldTrialServiceJobURL (job_p, data_p -> dftsd_view_programme_url_s, id_s);
												}

											FreeBSONOidString (id_s);
										}
									else
										{
											PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Failed to get bson oid string for trial \"%s\"", programme_p -> pr_name_s);
										}

								}

							FreeMongoTool (mongo_p);
						}		/* if (mongo_p) */
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "GetConfiguredMongoTool () failed");
						}




					json_decref (programme_json_p);
				}		/* if (programme_json_p) */

		}		/* if (success_flag) */

	SetServiceJobStatus (job_p, status);

	return status;
}



bool RemoveProgrammeFieldTrial (Programme *programme_p, FieldTrial *trial_p)
{
	bool removed_flag = false;

	if (programme_p -> pr_trials_p)
		{
			FieldTrialNode *node_p = (FieldTrialNode *) (programme_p -> pr_trials_p -> ll_head_p);

			while (node_p && !removed_flag)
				{
					if (node_p -> ftn_field_trial_p == trial_p)
						{
							LinkedListRemove (programme_p -> pr_trials_p, & (node_p -> ftn_node));

							node_p -> ftn_field_trial_p = NULL;
							FreeStudyNode (& (node_p -> ftn_node));
							removed_flag = true;
						}


					if (!removed_flag)
						{
							node_p = (FieldTrialNode *) (node_p -> ftn_node.ln_next_p);
						}
				}
		}


	return removed_flag;
}


Programme *GetVersionedProgramme (const char *programme_id_s, const char *timestamp_s, const ViewFormat format, const FieldTrialServiceData *data_p)
{
	void *(*callback_fn) (json_t *json_p, ViewFormat format, const FieldTrialServiceData *data_p) =
			(void * (*) (json_t *, ViewFormat, const FieldTrialServiceData *)) GetProgrammeFromJSON;

	Programme *programme_p = (Programme *) GetVersionedObject (programme_id_s, timestamp_s, format, DFTD_PROGRAMME, data_p, callback_fn);
	return programme_p;
}


void SetProgrammeUser (Programme *programme_p, User *user_p, bool owns_user_flag)
{
	SetMetadataUser (programme_p -> pr_metadata_p, user_p, owns_user_flag);
}


/*

{
  "@context": "https://schema.org",
  "@type": "Person",
  "name": "Turner, Caroline B.",
  "givenName": "Caroline B.",
  "familyName": "Turner",
  "funding": {
     "@type": "Grant",
     "identifier": "1448821",
     "funder": {
       "@type": "Organization",
       "name": "National Science Foundation",
       "identifier": "https://doi.org/10.13039/100000001"
     }
   }
}
 */


static bool AddFundingToClientProgrammeJSON (const Programme *programme_p, json_t *programme_json_p)
{
	json_t *grant_p = json_object ();

	if (grant_p)
		{
			json_t *funding_p = json_object ();

			if (funding_p)
				{
					if (json_object_set_new (grant_p, PR_GRANT_FUNDER_S, funding_p) == 0)
						{
							if (SetJSONString (grant_p, "@type", "Grant"))
								{
									if ((programme_p -> pr_project_code_s == NULL) || (SetJSONString (grant_p, PR_FUNDING_IDENTIFIER_S, programme_p -> pr_project_code_s)))
										{
											if (SetJSONString (funding_p, "@type", "Organization"))
												{
													if ((programme_p -> pr_funding_organisation_s == NULL) || (SetJSONString (funding_p, PR_FUNDING_ORG_NAME_S, programme_p -> pr_funding_organisation_s)))
														{
															if (json_object_set_new (programme_json_p, PR_FUNDING_S, grant_p) == 0)
																{
																	return true;
																}
															else
																{
																	PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, programme_json_p, "Failed to add funding object to Programme");
																}
														}		/* if ((programme_p -> pr_funding_organistaion_s == NULL) || (SetJSONString (grant_p, S_FUNDING_ORG_NAME_S, programme_p -> pr_funding_organistaion_s))) */
													else
														{
															PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, grant_p, "Failed to add \"%s\": \"%s\"", PR_FUNDING_ORG_NAME_S, programme_p -> pr_funding_organisation_s);
														}
												}		/* if (SetJSONString (grant_p, "@type", "Grant")) */
											else
												{
													PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, grant_p, "Failed to add \"@type\": \"Organization\"");
												}

										}		/* if ((programme_p -> pr_project_code_s == NULL) || (SetJSONString (grant_p, S_FUNDING_IDENTIFIER_S, programme_p -> pr_project_code_s))) */
									else
										{
											PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, grant_p, "Failed to add \"%s\": \"%s\"", PR_FUNDING_IDENTIFIER_S, programme_p -> pr_project_code_s);
										}
								}		/* if (SetJSONString (grant_p, "@type", "Grant")) */
							else
								{
									PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, grant_p, "Failed to add \"@type\": \"Grant\"");
								}

						}		/* if (json_object_set_new (grant_p, S_GRANT_FUNDER_S, funding_p) == 0) */
					else
						{
							PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, funding_p, "Failed to grant object");
							json_decref (funding_p);
						}
				}
			else
				{
					PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, programme_json_p, "Failed to allocate funding object");
				}

			json_decref (grant_p);
		}
	else
		{
			PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, programme_json_p, "Failed to allocate grant object");
		}

	return false;
}


static bool AddFullDetailsToProgrammeJSON (const Programme *programme_p, json_t *programme_json_p, const bool add_id_flag)
{
	if (SetNonTrivialString (programme_json_p, PR_DOCUMENTATION_URL_S, programme_p -> pr_documentation_url_s, true))
		{
			if (SetNonTrivialString (programme_json_p, PR_OBJECTIVE_S, programme_p -> pr_objective_s, true))
				{
					if (SetNonTrivialString (programme_json_p, PR_ABBREVIATION_S, programme_p -> pr_abbreviation_s, true))
						{
							if ((!add_id_flag) || (AddCompoundIdToJSON (programme_json_p, programme_p -> pr_id_p)))
								{
									return true;
								}
						}
				}
		}

	return false;
}



static bool AddPIToJSON (json_t *programme_json_p, const Person *person_p, const ViewFormat format, const FieldTrialServiceData *data_p)
{
	bool success_flag = false;
	json_t *pi_json_p = GetPersonAsJSON (person_p, format, data_p);

	if (pi_json_p)
		{
			if (json_object_set_new (programme_json_p, PR_PI_S, pi_json_p) == 0)
				{
					success_flag = true;
				}
			else
				{
					PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, programme_json_p, "Failed to add Person \"%s\" as JSON to Programme", person_p -> pe_name_s);
					json_decref (pi_json_p);
				}
		}
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to get Person \"%s\" as JSON", person_p -> pe_name_s);
		}

	return success_flag;
}
