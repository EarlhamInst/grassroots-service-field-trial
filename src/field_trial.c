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
 * field_trial.c
 *
 *  Created on: 11 Sep 2018
 *      Author: billy
 */


#define ALLOCATE_FIELD_TRIAL_TAGS (1)
#include "field_trial.h"
#include "field_trial_mongodb.h"
#include "dfw_field_trial_service_data.h"
#include "string_utils.h"
#include "study.h"
#include "memory_allocations.h"
#include "streams.h"
#include "dfw_util.h"
#include "indexing.h"
#include "json_processor.h"
#include "programme.h"
#include "person_jobs.h"
#include "mongodb_util.h"


static bool AddPersonFromJSON (Person *person_p, void *user_data_p, MEM_FLAG *mem_p);




FieldTrial *AllocateFieldTrial (const char *name_s, const char *team_s, Programme *parent_program_p, MEM_FLAG parent_program_mem, bson_oid_t *id_p, const char *time_s)
{
	char *copied_name_s = EasyCopyToNewString (name_s);

	if (copied_name_s)
		{
			char *copied_team_s = NULL;

			if ((team_s == NULL) || (copied_team_s = EasyCopyToNewString (team_s)))
				{
					char *copied_time_s = NULL;

					if ((time_s == NULL) || (copied_time_s = EasyCopyToNewString (time_s)))
						{
							LinkedList *studies_p = AllocateLinkedList (FreeStudyNode);

							if (studies_p)
								{
									LinkedList *people_p = AllocateLinkedList (FreePersonNode);

									if (people_p)
										{
											FieldTrial *trial_p = (FieldTrial *) AllocMemory (sizeof (FieldTrial));

											if (trial_p)
												{
													trial_p -> ft_name_s = copied_name_s;
													trial_p -> ft_team_s = copied_team_s;
													trial_p -> ft_id_p = id_p;
													trial_p -> ft_timestamp_s = copied_time_s;
													trial_p -> ft_studies_p = studies_p;
													trial_p -> ft_people_p = people_p;
													trial_p -> ft_parent_p = parent_program_p;
													trial_p -> ft_parent_program_mem = parent_program_mem;

													return trial_p;
												}

											FreeLinkedList (people_p);
										}		/* if (people_p) */

									FreeLinkedList (studies_p);
								}		/* if (studies_p) */


							if (copied_time_s)
								{
									FreeCopiedString (copied_time_s);
								}

						}		/* if ((time_s == NULL) || (copied_time_s = EasyCopyToNewString (time_s))) */


					if (copied_team_s)
						{
							FreeCopiedString (copied_team_s);
						}
				}		/* if (copied_team_s) */

			FreeCopiedString (copied_name_s);
		}		/* if (copied_name_s) */

	return NULL;
}



bool GenerateIdFromString (bson_oid_t **id_pp, const char *id_s)
{
	bool success_flag = false;

	if (bson_oid_is_valid (id_s, strlen (id_s)))
		{
			bson_oid_t *id_p = GetNewUnitialisedBSONOid ();

			if (id_p)
				{
					bson_oid_init_from_string (id_p, id_s);
					*id_pp = id_p;
					success_flag = true;
				}
		}

	return success_flag;
}


char *GetFieldTrialIdAsString (const FieldTrial *trial_p)
{
	char *id_s = NULL;

	if (trial_p -> ft_id_p)
		{
			id_s = GetBSONOidAsString (trial_p -> ft_id_p);
		}

	return id_s;
}


bool SaveFieldTrial (FieldTrial *trial_p, ServiceJob *job_p, FieldTrialServiceData *data_p)
{
	bson_t *selector_p = NULL;
	bool success_flag = PrepareSaveData (& (trial_p -> ft_id_p), &selector_p);
	OperationStatus status = OS_FAILED;

	if (success_flag)
		{
			json_t *field_trial_json_p = GetFieldTrialAsJSON (trial_p, VF_STORAGE, data_p);

			if (field_trial_json_p)
				{
					MongoTool *mongo_p = GetConfiguredMongoTool (data_p, NULL);

					if (mongo_p)
						{
							if (SaveAndBackupMongoDataWithTimestamp (mongo_p, field_trial_json_p, data_p -> dftsd_collection_ss [DFTD_FIELD_TRIAL], data_p -> dftsd_backup_collection_ss [DFTD_FIELD_TRIAL], DFT_BACKUPS_ID_KEY_S,  selector_p, MONGO_TIMESTAMP_S))
								{
									char *id_s = GetBSONOidAsString (trial_p -> ft_id_p);
									status = IndexData (job_p, field_trial_json_p, NULL);

									if (status != OS_SUCCEEDED)
										{
											PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, field_trial_json_p, "Failed to index FieldTrial \"%s\" as JSON to Lucene", trial_p -> ft_name_s);
											AddGeneralErrorMessageToServiceJob (job_p, "Trial saved but failed to index for searching");

											status = OS_PARTIALLY_SUCCEEDED;
										}

									if (id_s)
										{
											/*
											 * If we have the front-end web address to view the trial,
											 * save it to the ServiceJob.
											 */
											if (data_p -> dftsd_view_trial_url_s)
												{
													SetFieldTrialServiceJobURL (job_p, data_p -> dftsd_view_trial_url_s, id_s);
												}

											FreeBSONOidString (id_s);
										}
									else
										{
											PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Failed to get bson oid string for trial \"%s\"", trial_p -> ft_name_s);
										}


								}		/* if (SaveMongoData (data_p -> dftsd_mongo_p, field_trial_json_p, data_p -> dftsd_collection_ss [DFTD_FIELD_TRIAL], selector_p)) */

							FreeMongoTool (mongo_p);
						}
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "GetConfiguredMongoTool () failed");
						}



					json_decref (field_trial_json_p);
				}		/* if (field_trial_json_p) */

		}		/* if (success_flag) */

	SetServiceJobStatus (job_p, status);

	return success_flag;
}



void FreeFieldTrial (FieldTrial *trial_p)
{
	FreeCopiedString (trial_p -> ft_name_s);
	FreeCopiedString (trial_p -> ft_team_s);

	if (trial_p -> ft_id_p)
		{
			FreeBSONOid (trial_p -> ft_id_p);
		}

	if (trial_p -> ft_studies_p)
		{
			FreeLinkedList (trial_p -> ft_studies_p);
		}

	if (trial_p -> ft_people_p)
		{
			FreeLinkedList (trial_p -> ft_people_p);
		}


	if (trial_p -> ft_parent_p)
		{
			if ((trial_p -> ft_parent_program_mem == MF_DEEP_COPY) || (trial_p -> ft_parent_program_mem == MF_SHALLOW_COPY))
				{
					RemoveProgrammeFieldTrial (trial_p -> ft_parent_p, trial_p);

					if (GetNumberOfProgrammeFieldTrials (trial_p -> ft_parent_p) == 0)
						{
							FreeProgramme (trial_p -> ft_parent_p);
						}
				}
		}

	if (trial_p -> ft_timestamp_s)
		{
			FreeCopiedString (trial_p -> ft_timestamp_s);
		}

	FreeMemory (trial_p);
}


uint32 GetNumberOfFieldTrialStudies (const FieldTrial *trial_p)
{
	return (trial_p -> ft_studies_p ? trial_p -> ft_studies_p -> ll_size : 0);
}


bool RemoveFieldTrialStudy (FieldTrial *trial_p, Study *study_p)
{
	bool removed_flag = false;

	if (trial_p -> ft_studies_p)
		{
			StudyNode *node_p = (StudyNode *) (trial_p -> ft_studies_p -> ll_head_p);

			while (node_p && !removed_flag)
				{
					if (node_p -> stn_study_p == study_p)
						{
							LinkedListRemove (trial_p -> ft_studies_p, & (node_p -> stn_node));

							node_p -> stn_study_p = NULL;
							FreeStudyNode (& (node_p -> stn_node));
							removed_flag = true;
						}


					if (!removed_flag)
						{
							node_p = (StudyNode *) (node_p -> stn_node.ln_next_p);
						}
				}
		}


	return removed_flag;
}


LinkedList *GetFieldTrialsByName (const char * const trial_s, const FieldTrialServiceData *data_p)
{
	LinkedList *trials_p = GetFieldTrialsByNameFromMongoDB (data_p, trial_s);

	return trials_p;
}



json_t *GetFieldTrialAsJSON (FieldTrial *trial_p, const ViewFormat format, FieldTrialServiceData *data_p)
{
	json_t *trial_json_p = json_object ();

	if (trial_json_p)
		{
			if (SetNonTrivialString (trial_json_p, FT_NAME_S, trial_p -> ft_name_s, true))
				{
					if (SetNonTrivialString (trial_json_p, FT_TEAM_S, trial_p -> ft_team_s, true))
						{
							if (SetNonTrivialString (trial_json_p, MONGO_TIMESTAMP_S, trial_p -> ft_timestamp_s, true))
								{
									if (AddCompoundIdToJSON (trial_json_p, trial_p -> ft_id_p))
										{
											bool success_flag = false;

											if (AddPeopleToJSON (trial_p -> ft_people_p, FT_PEOPLE_S, trial_json_p, format, data_p))
												{
													if ((format == VF_CLIENT_FULL) || (format == VF_CLIENT_MINIMAL))
														{
															if (trial_p -> ft_parent_p)
																{
																	json_t *program_p = GetProgrammeAsJSON (trial_p -> ft_parent_p, format, data_p);

																	if (program_p)
																		{
																			if (json_object_set_new (trial_json_p, FT_PARENT_PROGRAM_S, program_p) == 0)
																				{
																					success_flag = true;
																				}		/* if (json_object_set_new (trial_json_p, FT_PARENT_PROGRAM_S, program_p) == 0) */
																			else
																				{
																					PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, trial_json_p, "Failed to add program \"%s\" to json", trial_p -> ft_parent_p -> pr_name_s);
																					json_decref (program_p);
																				}
																		}		/* if (program_p) */
																	else
																		{
																			PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, trial_json_p, "Failed to get json for program \"%s\" in format %d", trial_p -> ft_parent_p -> pr_name_s, format);
																		}

																}		/* if (trial_p -> ft_parent_p) */
															else
																{
																	PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "trial \"%s\" has no parent program", trial_p -> ft_name_s);
																}

															if (format == VF_CLIENT_FULL)
																{
																	if (!AddStudiesToFieldTrialJSON (trial_p, trial_json_p, format, data_p))
																		{
																			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "AddStudiesToFieldTrialJSON () failed for trial \"%s\"", trial_p -> ft_name_s);
																			success_flag = false;
																		}		/* if (!AddStudiesToFieldTrialJSON (trial_p, trial_json_p, format, data_p)) */
																}
														}
													else if (format == VF_STORAGE)
														{
															if ((! (trial_p -> ft_parent_p)) || (AddNamedCompoundIdToJSON (trial_json_p, trial_p -> ft_parent_p -> pr_id_p, FT_PARENT_PROGRAM_S)))
																{
																	success_flag = true;
																}

														}

												}		/* if (AddPeopleToFieldTrialJSON (trial_p, trial_json_p, format, data_p)) */
											else
												{

												}


											if (success_flag)
												{
													if (AddDatatype (trial_json_p, DFTD_FIELD_TRIAL))
														{
															return trial_json_p;
														}
												}
										}

								}		/* if (SetNonTrivialString (trial_json_p, DFT_TIMESTAMP_S, trial_p -> ft_timestamp_s, true)) */
							else
								{
									PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, trial_json_p, "Failed to set \"%s\": \"%s\"", MONGO_TIMESTAMP_S, trial_p -> ft_timestamp_s ?  trial_p -> ft_timestamp_s : "null");
								}

						}		/* if (json_object_set_new (trial_json_p, team_key_s, json_string (trial_p -> ft_team_s)) == 0) */

				}		/* if (json_object_set_new (trial_json_p, name_key_s, json_string (trial_p -> ft_name_s)) == 0) */

			json_decref (trial_json_p);
		}		/* if (trial_json_p) */

	return NULL;
}



bool AddStudiesToFieldTrialJSON (FieldTrial *trial_p, json_t *trial_json_p, const ViewFormat format, FieldTrialServiceData *data_p)
{
	bool success_flag = true;

	if (trial_p -> ft_studies_p -> ll_size > 0)
		{
			json_t *studies_p = json_array ();

			if (studies_p)
				{
					StudyNode *node_p = (StudyNode *) (trial_p -> ft_studies_p -> ll_head_p);
					bool ok_flag = true;
					JSONProcessor *processor_p = NULL;

					while (node_p && ok_flag)
						{
							json_t *study_p = GetStudyAsJSON (node_p -> stn_study_p, format, processor_p, data_p);

							if (study_p)
								{
									if (json_array_append_new (studies_p, study_p) == 0)
										{
											node_p = (StudyNode *) (node_p -> stn_node.ln_next_p);
										}
									else
										{
											ok_flag = false;
											PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, study_p, "Failed to add Study json to array for field trial \"%s\" - \"%s\"", trial_p -> ft_team_s, trial_p -> ft_name_s);
											json_decref (study_p);
										}
								}
							else
								{
									char buffer_s [MONGO_OID_STRING_BUFFER_SIZE];

									ok_flag = false;
									bson_oid_to_string (node_p -> stn_study_p -> st_id_p, buffer_s);

									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to get Study json for study \"%s\"", buffer_s);
								}
						}		/* while (node_p && success_flag) */

					if (ok_flag)
						{
							if (json_object_set_new (trial_json_p, FT_STUDIES_S, studies_p) == 0)
								{
									success_flag = true;
								}
							else
								{
									char buffer_s [MONGO_OID_STRING_BUFFER_SIZE];

									ok_flag = false;

									bson_oid_to_string (trial_p -> ft_id_p, buffer_s);

									PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, studies_p, "Failed to add Study json to trial \"%s\"", buffer_s);

									json_decref (studies_p);
								}
						}

				}		/* if (exp_areas_p) */
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate Studies json object for field trial \"%s\" - \"%s\"", trial_p -> ft_team_s, trial_p -> ft_name_s);
				}

		}		/* if (trial_p -> ft_studies_p -> ll_size > 0) */
	else
		{
			/* nothing to add */
			success_flag = true;
		}

	return success_flag;
}


FieldTrial *GetFieldTrialFromJSON (const json_t *json_p, const ViewFormat format, const FieldTrialServiceData *data_p)
{
	FieldTrial *trial_p = NULL;
	const char *name_s = GetJSONString (json_p, FT_NAME_S);

	if (name_s)
		{
			const char *team_s = GetJSONString (json_p, FT_TEAM_S);
			bson_oid_t *id_p = GetNewUnitialisedBSONOid ();

			if (id_p)
				{
					if (GetMongoIdFromJSON (json_p, id_p))
						{
							bson_oid_t *program_id_p = GetNewUnitialisedBSONOid ();

							if (program_id_p)
								{
									bool success_flag = true;
									Programme *program_p = NULL;

									if (GetNamedIdFromJSON (json_p, FT_PARENT_PROGRAM_S, program_id_p))
										{
											if (! (program_p = GetProgrammeById (program_id_p, format, data_p)))
												{
													char *id_s = GetBSONOidAsString (program_id_p);

													if (id_s)
														{
															PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, json_p, "GetProgramById failed for %s", id_s);
															FreeBSONOidString (id_s);
														}
													else
														{
															PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, json_p, "GetProgramById failed");
														}

													success_flag = false;
												}

										}		/* if (GetNamedIdFromJSON (json_p, FT_PARENT_PROGRAM_S, program_id_p)) */


									if (success_flag)
										{
											const char *timestamp_s = GetJSONString (json_p, MONGO_TIMESTAMP_S);
											trial_p = AllocateFieldTrial (name_s, team_s, program_p, MF_SHALLOW_COPY, id_p, timestamp_s);

											if (trial_p)
												{
													const json_t *people_json_p = json_object_get (json_p, FT_PEOPLE_S);

													if (people_json_p)
														{
															AddPeopleFromJSON (people_json_p, AddPersonFromJSON, trial_p, data_p);
														}

												}
										}


									FreeBSONOid (program_id_p);
								}		/* if (program_id_p) */

						}

					if (!trial_p)
						{
							FreeBSONOid (id_p);
						}

				}		/* if (id_p) */

		}

	return trial_p;
}


FieldTrialNode *AllocateFieldTrialNode (FieldTrial *trial_p)
{
	FieldTrialNode *node_p = (FieldTrialNode *) AllocMemory (sizeof (FieldTrialNode));

	if (node_p)
		{
			InitListItem (& (node_p -> ftn_node));
			node_p -> ftn_field_trial_p = trial_p;
		}

	return node_p;
}


void FreeFieldTrialNode (ListItem *node_p)
{
	FieldTrialNode *field_trial_node_p = (FieldTrialNode *) node_p;

	if (field_trial_node_p -> ftn_field_trial_p)
		{
			FreeFieldTrial (field_trial_node_p -> ftn_field_trial_p);
		}

	FreeMemory (field_trial_node_p);
}


/*
 * The trial could be the bson_oid or a name so check
 */

FieldTrial *GetUniqueFieldTrialBySearchString (const char *trial_s, const ViewFormat format, const FieldTrialServiceData *data_p)
{
	FieldTrial *trial_p = NULL;

	if (bson_oid_is_valid (trial_s, strlen (trial_s)))
		{
			trial_p = GetFieldTrialByIdString (trial_s, format, data_p);
		}

	if (!trial_p)
		{
			LinkedList *trials_p = GetFieldTrialsByName (trial_s, data_p);

			if (trials_p)
				{
					if (trials_p -> ll_size == 1)
						{
							FieldTrialNode *node_p = (FieldTrialNode *) (trials_p -> ll_head_p);

							/* Remove the trial from the node */
							trial_p = node_p -> ftn_field_trial_p;
							node_p -> ftn_field_trial_p = NULL;
						}

					FreeLinkedList (trials_p);
				}
		}


	return trial_p;
}


FieldTrial *GetFieldTrialById (const bson_oid_t *id_p, const ViewFormat format, const FieldTrialServiceData *data_p)
{
	FieldTrial *trial_p = NULL;
	char *id_s = GetBSONOidAsString (id_p);

	if (id_s)
		{
			trial_p = GetFieldTrialByIdString (id_s, format, data_p);

			FreeBSONOidString (id_s);
		}

	return trial_p;
}


FieldTrial *GetVersionedFieldTrial (const char *field_trial_id_s, const char *timestamp_s, const ViewFormat format, const FieldTrialServiceData *data_p)
{
	void *(*callback_fn) (json_t *json_p, ViewFormat format, const FieldTrialServiceData *data_p) =
			(void * (*) (json_t *, ViewFormat, const FieldTrialServiceData *)) GetFieldTrialFromJSON;

	FieldTrial *trial_p = GetVersionedObject (field_trial_id_s, timestamp_s, format, DFTD_FIELD_TRIAL, data_p, callback_fn);
	return trial_p;
}


FieldTrial *GetFieldTrialByIdString (const char *field_trial_id_s, const ViewFormat format, const FieldTrialServiceData *data_p)
{
	FieldTrial *trial_p = NULL;
	MongoTool *mongo_p = GetConfiguredMongoTool (data_p, NULL);

	if (mongo_p)
		{
			if (bson_oid_is_valid (field_trial_id_s, strlen (field_trial_id_s)))
				{
					if (SetMongoToolCollection (mongo_p, data_p -> dftsd_collection_ss [DFTD_FIELD_TRIAL]))
						{
							bson_t *query_p = bson_new ();

							if (query_p)
								{
									bson_oid_t oid;

									bson_oid_init_from_string (&oid, field_trial_id_s);

									if (BSON_APPEND_OID (query_p, MONGO_ID_S, &oid))
										{
											json_t *results_p = GetAllMongoResultsAsJSON (mongo_p, query_p, NULL);

											if (results_p)
												{
													if (json_is_array (results_p))
														{
															size_t num_results = json_array_size (results_p);

															if (num_results == 1)
																{
																	json_t *res_p = json_array_get (results_p, 0);

																	trial_p = GetFieldTrialFromJSON (res_p, format, data_p);

																	if (!trial_p)
																		{
																			PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, res_p, "failed to create field trial for id \"%s\"", field_trial_id_s);
																		}

																}		/* if (num_results == 1) */
															else
																{
																	PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, results_p, "" SIZET_FMT " results when searching for field trial with id \"%s\"", num_results, field_trial_id_s);
																}

														}		/* if (json_is_array (results_p) */
													else
														{
															PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, results_p, "Results are not an array");
														}

													json_decref (results_p);
												}		/* if (results_p) */
											else
												{
													PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to get results searching for field trial with id \"%s\"", field_trial_id_s);
												}

										}		/* if (BSON_APPEND_OID (query_p, MONGO_ID_S, &oid)) */
									else
										{
											PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to create query for field trial with id \"%s\"", field_trial_id_s);
										}

									bson_destroy (query_p);
								}		/* if (query_p) */
							else
								{
									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to create query for field trial with id \"%s\"", field_trial_id_s);
								}

						}		/* if (SetMongoToolCollection (tool_p, data_p -> dftsd_collection_ss [DFTD_FIELD_TRIAL])) */
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to set collection to \"%s\"", data_p -> dftsd_collection_ss [DFTD_FIELD_TRIAL]);
						}

				}		/* if (bson_oid_is_valid (field_trial_id_s, strlen (field_trial_id_s))) */
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "\"%s\" is not a valid oid", field_trial_id_s);
				}

			FreeMongoTool (mongo_p);
		}		/* if (mongo_p) */
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "GetConfiguredMongoTool () failed");
		}


	return trial_p;
}




bool AddFieldTrialStudy (FieldTrial *trial_p, Study *study_p, MEM_FLAG mf)
{
	bool success_flag = false;
	StudyNode *node_p = AllocateStudyNode (study_p);

	if (node_p)
		{
			if (study_p -> st_parent_p)
				{

				}

			study_p -> st_parent_p = trial_p;
			study_p -> st_parent_field_trial_mem = mf;

			LinkedListAddTail (trial_p -> ft_studies_p, & (node_p -> stn_node));
			success_flag  = true;
		}

	return success_flag;
}


bool AddFieldTrialPerson (FieldTrial *trial_p, Person *person_p, MEM_FLAG mf)
{
	bool success_flag = false;
	PersonNode *node_p = AllocatePersonNode (person_p);

	if (node_p)
		{
			LinkedListAddTail (trial_p -> ft_people_p, & (node_p -> pn_node));
			success_flag  = true;
		}

	return success_flag;
}



bool GetAllFieldTrialStudies (FieldTrial *trial_p, const ViewFormat format, const FieldTrialServiceData *data_p)
{
	bool success_flag = false;

	bson_t *query_p = bson_new ();

	if (query_p)
		{
			if (BSON_APPEND_OID (query_p, ST_PARENT_FIELD_TRIAL_S, trial_p -> ft_id_p))
				{
					MongoTool *mongo_p = GetConfiguredMongoTool (data_p, NULL);

					if (mongo_p)
						{
							if (SetMongoToolCollection (mongo_p, data_p -> dftsd_collection_ss [DFTD_STUDY]))
								{
									bson_t *opts_p = BCON_NEW ( "sort", "{", ST_HARVEST_YEAR_S, BCON_INT32 (1), "}");

									if (opts_p)
										{
											json_t *results_p = GetAllMongoResultsAsJSON (mongo_p, query_p, opts_p);

											if (results_p)
												{
													if (json_is_array (results_p))
														{
															const size_t num_results = json_array_size (results_p);

															success_flag = true;

															if (num_results > 0)
																{
																	size_t i;
																	json_t *study_json_p;

																	json_array_foreach (results_p, i, study_json_p)
																		{
																			Study *study_p = GetStudyWithParentTrialFromJSON (study_json_p, trial_p, format, data_p);

																			if (study_p)
																				{
																					if (!AddFieldTrialStudy (trial_p, study_p, MF_SHADOW_USE))
																						{
																							FreeStudy (study_p);
																							success_flag = false;
																						}
																				}
																			else
																				{
																					success_flag = false;
																				}
																		}		/* json_array_foreach (results_p, i, study_json_p) */

																}
														}

													json_decref (results_p);
												}		/* if (results_p) */

											bson_destroy (opts_p);
										}		/* if (opts_p) */

								}		/* if (SetMongoToolCollection (data_p -> dftsd_mongo_p, data_p -> dftsd_collection_ss [DFTD_EXPERIMENTAL_AREA])) */

							FreeMongoTool (mongo_p);
						}		/* if (mongo_p) */
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "GetConfiguredMongoTool () failed");
						}

				}		/* if (BSON_APPEND_OID (query_p, ST_PARENT_FIELD_TRIAL_S, trial_p -> ft_id_p)) */

			bson_destroy (query_p);
		}		/* if (query_p) */


	return success_flag;
}



char *GetFieldTrialAsString (const FieldTrial *trial_p)
{
	char *trial_s = NULL;

	if (! (trial_p -> ft_team_s))
		{
			trial_s = EasyCopyToNewString (trial_p -> ft_name_s);
		}
	else
		{
			trial_s = ConcatenateVarargsStrings (trial_p -> ft_team_s, " - ", trial_p -> ft_name_s, NULL);

			if (!trial_s)
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to get trial as string: team \"%s\", name \"%s\"", 
											(trial_p -> ft_team_s != NULL) ? trial_p -> ft_team_s : "NULL",
											(trial_p -> ft_name_s != NULL) ? trial_p -> ft_name_s : "NULL");
				}
		}


	return trial_s;
}

static bool AddPersonFromJSON (Person *person_p, void *user_data_p, MEM_FLAG *mem_p)
{
	bool success_flag = false;
	FieldTrial *trial_p = (FieldTrial *) user_data_p;
	MEM_FLAG mf = MF_SHALLOW_COPY;

	if (AddFieldTrialPerson (trial_p, person_p, mf))
		{
			*mem_p = mf;
			success_flag = true;
		}

	return success_flag;
}
