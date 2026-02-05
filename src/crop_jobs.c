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
 * crop_jobs.c
 *
 *  Created on: 17 Apr 2019
 *      Author: billy
 */



#include "crop.h"
#include "crop_jobs.h"
#include "string_utils.h"
#include "bson/bson.h"

#include "string_parameter.h"


/*
 * static declarations
 */

static NamedParameterType S_NAME = { "CR Name", PT_KEYWORD };
static NamedParameterType S_PREFERRED_TERM = { "CR Preferred Term", PT_KEYWORD};
static NamedParameterType S_ONTOLOGY_URL = { "CR Ontology URL", PT_STRING };
static NamedParameterType S_SYNONYMS = { "CR Synonyms", PT_LARGE_STRING };


static bool SetDefaultCropValue (StringParameter *param_p, const char *crop_id_s);


/*
 * API definitions
 */

bool AddSubmissionCropParams (ServiceData *data_p, ParameterSet *param_set_p)
{
	bool success_flag = false;
	ParameterGroup *group_p = CreateAndAddParameterGroupToParameterSet ("Crops", false, data_p, param_set_p);

	if (group_p)
		{
			Parameter *param_p = NULL;

			if ((param_p = EasyCreateAndAddStringParameterToParameterSet (data_p, param_set_p, group_p, S_NAME.npt_type, S_NAME.npt_name_s, "Name", "The crop name", NULL, PL_ALL)) != NULL)
				{
					param_p -> pa_required_flag = true;

					if ((param_p = EasyCreateAndAddStringParameterToParameterSet (data_p, param_set_p, group_p, S_PREFERRED_TERM.npt_type, S_PREFERRED_TERM.npt_name_s, "Preferred name", "The AgroVOC preferred term for the crop", NULL, PL_ALL)) != NULL)
						{
							if ((param_p = EasyCreateAndAddStringParameterToParameterSet (data_p, param_set_p, group_p, S_ONTOLOGY_URL.npt_type, S_ONTOLOGY_URL.npt_name_s, "Definition", "The URL of the ontology definition for this crop", NULL, PL_ALL)) != NULL)
								{
									param_p -> pa_required_flag = true;

									if ((param_p = EasyCreateAndAddStringParameterToParameterSet (data_p, param_set_p, group_p, S_SYNONYMS.npt_type, S_SYNONYMS.npt_name_s, "Synonyms", "The comma-separated list of synonyms for this crop", NULL, PL_ALL)) != NULL)
										{
											success_flag = true;
										}
								}
						}
				}
		}		/* if (group_p) */


	return success_flag;
}


bool RunForSubmissionCropParams (FieldTrialServiceData *data_p, ParameterSet *param_set_p, ServiceJob *job_p)
{
	bool job_done_flag = false;
	const char *name_s = NULL;

	if (GetCurrentStringParameterValueFromParameterSet (param_set_p, S_NAME.npt_name_s, &name_s))
		{
			/*
			 * The name is required
			 */
			if (! (IsStringEmpty (name_s)))
				{
					const char *term_s = NULL;
					OperationStatus status = OS_FAILED_TO_START;

					if (GetCurrentStringParameterValueFromParameterSet (param_set_p, S_PREFERRED_TERM.npt_name_s, &term_s))
						{
							const char *ontology_s = NULL;

							if (GetCurrentStringParameterValueFromParameterSet (param_set_p, S_ONTOLOGY_URL.npt_name_s, &ontology_s))
								{
									const char *synonyms_s = NULL;

									if (GetCurrentStringParameterValueFromParameterSet (param_set_p, S_SYNONYMS.npt_name_s, &synonyms_s))
										{
											bool success_flag = true;
											char **synonyms_ss = NULL;

											if (!IsStringEmpty (synonyms_s))
												{
													LinkedList *synonyms_p = ParseStringToStringLinkedList (synonyms_s, ",", false);

													if (synonyms_p)
														{
															synonyms_ss = (char **) AllocMemoryArray (1 + synonyms_p -> ll_size, sizeof (char *));

															if (synonyms_ss)
																{
																	StringListNode *node_p = (StringListNode *) synonyms_p -> ll_head_p;
																	char **synonym_ss = synonyms_ss;

																	while (node_p)
																		{
																			*synonym_ss = DetachStringFromStringListNode (node_p);

																			++ synonym_ss;
																			node_p = (StringListNode *) (node_p -> sln_node.ln_next_p);
																		}

																}

															FreeLinkedList (synonyms_p);
														}
													else
														{
															PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to parse \"%s\" for synonyms", synonyms_s);
															success_flag = false;
														}

												}		/* if (!IsStringEmpty (synonyms_value.st_string_value_s)) */

											if (success_flag)
												{
													Crop *crop_p = AllocateCrop (NULL, name_s, term_s, ontology_s, synonyms_ss);

													if (crop_p)
														{
															if (SaveCrop (crop_p, data_p))
																{
																	status = OS_SUCCEEDED;
																}
														}
													else
														{

															status = OS_FAILED;
															if (synonyms_ss)
																{
																	char **synonym_ss = synonyms_ss;

																	while (*synonym_ss)
																		{
																			FreeCopiedString (*synonym_ss);
																			++ synonym_ss;
																		}
																}
														}

												}

										}		/* if (GetParameterValueFromParameterSet (param_set_p, S_SYNONYMS.npt_name_s, &synonyms_value, true)) */
									else
										{
											PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to get \"%s\" parameter value", S_SYNONYMS.npt_name_s);
										}

								}		/* if (GetParameterValueFromParameterSet (param_set_p, S_ONTOLOGY_URL.npt_name_s, &ontology_value, true)) */
							else
								{
									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to get \"%s\" parameter value", S_ONTOLOGY_URL.npt_name_s);
								}

						}		/* if (GetParameterValueFromParameterSet (param_set_p, S_PREFERRED_TERM.npt_name_s, &term_value, true)) */
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to get \"%s\" parameter value", S_PREFERRED_TERM.npt_name_s);
						}

					job_done_flag = true;

					SetServiceJobStatus (job_p, status);

				}		/* if (! (IsStringEmpty (value.st_string_value_s))) */

		}		/* if (GetParameterValueFromParameterSet (param_set_p, S_NAME.npt_name_s, &name_value, true)) */
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to get \"%s\" parameter value", S_NAME.npt_name_s);
		}


	return job_done_flag;
}


bool GetSubmissionCropParameterTypeForNamedParameter (const char *param_name_s, ParameterType *pt_p)
{
	bool success_flag = true;

	if (strcmp (param_name_s, S_NAME.npt_name_s) == 0)
		{
			*pt_p = S_NAME.npt_type;
		}
	else if (strcmp (param_name_s, S_PREFERRED_TERM.npt_name_s) == 0)
		{
			*pt_p = S_PREFERRED_TERM.npt_type;
		}
	else if (strcmp (param_name_s, S_ONTOLOGY_URL.npt_name_s) == 0)
		{
			*pt_p = S_ONTOLOGY_URL.npt_type;
		}
	else if (strcmp (param_name_s, S_SYNONYMS.npt_name_s) == 0)
		{
			*pt_p = S_SYNONYMS.npt_type;
		}
	else
		{
			success_flag = false;
		}

	return success_flag;
}


bool SetUpCropsListParameter (const FieldTrialServiceData *data_p, Parameter *param_p, const Crop *active_crop_p, const char *empty_option_s, const bool new_study_flag)
{
	bool success_flag = false;

	MongoTool *mongo_p = GetConfiguredMongoTool (data_p, NULL);

	if (mongo_p)
		{
			if (SetMongoToolCollection (mongo_p, data_p -> dftsd_collection_ss [DFTD_CROP]))
				{
					bson_t *query_p = NULL;
					bson_t *opts_p =  BCON_NEW ( "sort", "{", CR_NAME_S, BCON_INT32 (1), "}", "collation", "{", "locale", BCON_UTF8 ("en"), "}");
					json_t *results_p = GetAllMongoResultsAsJSON (mongo_p, query_p, opts_p);

					if (results_p)
						{
							if (json_is_array (results_p))
								{
									const size_t num_results = json_array_size (results_p);

									if (num_results > 0)
										{
											size_t i;
											const json_t *service_config_p = data_p -> dftsd_base_data.sd_config_p;
											const char *default_crop_s = NULL;
											bool default_is_set_flag = false;

											if (active_crop_p)
												{
													default_crop_s = active_crop_p -> cr_name_s;
												}
											else if (new_study_flag)
												{
													default_crop_s = GetJSONString (service_config_p, "default_crop");
												}

											if ((!default_crop_s) && empty_option_s)
												{
													default_crop_s = empty_option_s;
												}

											/*
											 * If there's an empty option, add it
											 */
											if (empty_option_s)
												{
													success_flag = CreateAndAddStringParameterOption (param_p, empty_option_s, empty_option_s);

													if (!default_crop_s)
														{
															success_flag = SetDefaultCropValue ((StringParameter *) param_p, empty_option_s);

															default_is_set_flag = true;
														}
												}

											for (i = 0; i < num_results; ++ i)
												{
													json_t *entry_p = json_array_get (results_p, i);
													Crop *crop_p = GetCropFromJSON (entry_p, data_p);

													if (crop_p)
														{
															char *id_s = GetBSONOidAsString (crop_p -> cr_id_p);

															if (id_s)
																{
																	if (!default_is_set_flag)
																		{
																			if (default_crop_s)
																				{
																					if (strcmp (crop_p -> cr_name_s, default_crop_s) == 0)
																						{
																							success_flag = SetDefaultCropValue ((StringParameter *) param_p, id_s);
																							default_is_set_flag = true;
																						}
																				}
																			else if (i == 0)
																				{
																					success_flag = SetDefaultCropValue ((StringParameter *) param_p, id_s);
																					default_is_set_flag = true;
																				}
																		}

																	success_flag = CreateAndAddStringParameterOption (param_p, id_s, crop_p -> cr_name_s);

																	FreeBSONOidString (id_s);
																}

															FreeCrop (crop_p);
														}		/* if (crop_p) */

												}		/* for (i = 0; i < num_results; ++ i)) */

										}		/* if (num_results > 0) */
									else
										{
											/* nothing to add */
											success_flag = true;
										}

								}		/* if (json_is_array (results_p)) */

							json_decref (results_p);
						}		/* if (results_p) */

					if (opts_p)
						{
							bson_destroy (opts_p);
						}

				}		/* if (SetMongoToolCollection (data_p -> dftsd_mongo_p, data_p -> dftsd_collection_ss [DFTD_FIELD_TRIAL])) */

			FreeMongoTool (mongo_p);
		}		/* if (mongo_p) */
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "GetConfiguredMongoTool () failed");
		}


	return success_flag;
}



Crop *GetStoredCropValue (const json_t *json_p, const char *key_s, const FieldTrialServiceData *data_p)
{
	Crop *crop_p = NULL;
	const json_t *child_p = json_object_get (json_p, key_s);

	/*
	 * Is there a stored value?
	 */
	if ((child_p != NULL) && (! (json_is_null (child_p))))
		{
			bson_oid_t *crop_id_p = GetNewUnitialisedBSONOid ();

			if (crop_id_p)
				{
					if (GetNamedIdFromJSON (json_p, key_s, crop_id_p))
						{
							char *id_s = GetBSONOidAsString (crop_id_p);

							if (id_s)
								{
									crop_p = GetCropByIdString (id_s, data_p);

									if (!crop_p)
										{
											PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "GetCropByIdString failed for \"%s\"", id_s);
										}

									FreeBSONOidString (id_s);
								}		/* if (id_s) */
							else
								{
									PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, json_p, "GetBSONOidAsString for \"%s\"", key_s);
								}

						}		/* if (GetNamedIdFromJSON (json_p, key_s, crop_id_p)) */
					else
						{
							PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, json_p, "GetNamedIdFromJSON failed for \"%s\"", key_s);
						}

					FreeBSONOid (crop_id_p);
				}		/* if (crop_id_p) */
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "GetNewUnitialisedBSONOid failed");
				}

		}		/* if (json_object_get (json_p, key_s)) */


	return crop_p;
}




static bool SetDefaultCropValue (StringParameter *param_p, const char *crop_id_s)
{
	bool success_flag = false;

	if (SetStringParameterCurrentValue (param_p, crop_id_s))
		{
			if (SetStringParameterDefaultValue (param_p, crop_id_s))
				{
					success_flag = true;
				}
		}

	return success_flag;
}
