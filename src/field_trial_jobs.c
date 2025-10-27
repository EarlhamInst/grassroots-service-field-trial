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
 * field_trial_jobs.c
 *
 *  Created on: 1 Oct 2018
 *      Author: billy
 */

#define ALLOCATE_FIELD_TRIAL_CONSTANTS (1)
#include "field_trial_jobs.h"
#include "field_trial_mongodb.h"

#include "field_trial.h"
#include "string_utils.h"
#include "dfw_util.h"
#include "programme_jobs.h"
#include "boolean_parameter.h"
#include "person_jobs.h"
#include "frictionless_data_util.h"

/*
 * Field Trial parameters
 */

static NamedParameterType S_FUZZY_SEARCH_FIELD_TRIALS = { "Fuzzy Search", PT_BOOLEAN };
static NamedParameterType S_FULL_DATA = { "Get full data from search", PT_BOOLEAN };



static bool AddFieldTrialToServiceJobResult (ServiceJob *job_p, FieldTrial *trial_p, json_t *trial_json_p, const ViewFormat format, FieldTrialServiceData *data_p);

static bool SetUpDefaults (char **id_ss, char **program_id_ss, const char **name_ss, const char **team_ss, LinkedList **existing_people_pp);

static bool SetUpDefaultsFromExistingFieldTrial (const FieldTrial * const trial_p, char **id_ss, char **program_id_ss, const char **name_ss, const char **team_ss, LinkedList **existing_people_pp);


static bool ProcessPersonForFieldTrial (Person *person_p, void *user_data_p);





bool AddSubmissionFieldTrialParams (ServiceData *data_p, ParameterSet *param_set_p, FieldTrial *active_trial_p, const bool read_only_flag)
{
	FieldTrialServiceData *dfw_data_p = (FieldTrialServiceData *) data_p;
	bool success_flag = false;
	char *id_s = NULL;
	char *programme_id_s = NULL;
	const char *name_s = NULL;
	const char *team_s = NULL;
	LinkedList *existing_people_p = NULL;

	if (PopulaterActiveTrialValues (active_trial_p, &id_s, &programme_id_s, &name_s, &team_s, &existing_people_p, param_set_p, dfw_data_p))
		{
			ParameterGroup *group_p = CreateAndAddParameterGroupToParameterSet ("Main", false, & (dfw_data_p -> dftsd_base_data), param_set_p);

			if (AddTrialsList (id_s, param_set_p, group_p, read_only_flag, FT_EMPTY_LIST_OPTION_S, dfw_data_p))
				{
					if (AddTrialEditor (name_s, team_s, programme_id_s, existing_people_p, param_set_p, group_p, read_only_flag, dfw_data_p))
						{
							success_flag = true;
						}
				}

		}

	return success_flag;
}


bool PopulaterActiveTrialValues (FieldTrial *active_trial_p, char **id_ss, char **programme_id_ss, const char **name_ss, const char **team_ss, LinkedList **existing_people_pp, ParameterSet *param_set_p, FieldTrialServiceData *dfw_data_p)
{
	bool success_flag = false;

	if (active_trial_p)
		{
			if (SetUpDefaultsFromExistingFieldTrial (active_trial_p, id_ss, programme_id_ss, name_ss, team_ss, existing_people_pp))
				{
					success_flag = true;
				}
		}
	else
		{
			if (SetUpDefaults (id_ss, programme_id_ss, name_ss, team_ss, existing_people_pp))
				{
					success_flag = true;
				}
		}

	return success_flag;
}



bool AddTrialsListFromJSON (const char *id_s, json_t *trials_json_p, ParameterSet *param_set_p, ParameterGroup *group_p, const bool read_only_flag, const char *empty_option_s, FieldTrialServiceData *ft_data_p)
{
	bool success_flag = false;
	ServiceData *data_p = (ServiceData *) ft_data_p;
	Parameter *param_p = NULL;

	if ((param_p = EasyCreateAndAddStringParameterToParameterSet (data_p, param_set_p, group_p, FIELD_TRIAL_ID.npt_type, FIELD_TRIAL_ID.npt_name_s, "Load Field Trial", "Edit an existing Field Trial", id_s, PL_ALL)) != NULL)
		{
			param_p -> pa_read_only_flag = read_only_flag;

			if (SetUpListParameterFromJSON (ft_data_p, (StringParameter *) param_p, id_s, empty_option_s, FT_NAME_S, trials_json_p))
				{
					/*
					 * We want to update all of the values in the form
					 * when a user selects a study from the list so
					 * we need to make the parameter automatically
					 * refresh the values. So we set the
					 * pa_refresh_service_flag to true.
					 */
					param_p -> pa_refresh_service_flag = true;

					success_flag = true;
				}
		}

	return success_flag;
}


bool AddTrialsList (const char *id_s, ParameterSet *param_set_p, ParameterGroup *group_p, const bool read_only_flag, const char * const empty_option_s, FieldTrialServiceData *data_p)
{
	bool success_flag = false;
	json_t *trials_p = GetAllFieldTrialsAsJSON (data_p, false);

	if (trials_p)
		{
			success_flag = AddTrialsListFromJSON (id_s, trials_p, param_set_p, group_p, read_only_flag, empty_option_s, data_p);
			json_decref (trials_p);
		}

	return success_flag;
}


bool AddTrialEditor (const char * const name_s, const char * const team_s, const char * const programme_id_s, 	LinkedList *existing_people_p, ParameterSet *param_set_p, ParameterGroup *group_p, const bool read_only_flag, FieldTrialServiceData *dfw_data_p)
{
	bool success_flag = false;
	ServiceData *data_p = (ServiceData *) dfw_data_p;
	Parameter *param_p = NULL;

	if ((param_p = EasyCreateAndAddStringParameterToParameterSet (data_p, param_set_p, group_p, FIELD_TRIAL_NAME.npt_type, FIELD_TRIAL_NAME.npt_name_s, "Name", "The name of the Field Trial", name_s, PL_ALL)) != NULL)
		{
			param_p -> pa_required_flag = true;
			param_p -> pa_read_only_flag = read_only_flag;

			if ((param_p = EasyCreateAndAddStringParameterToParameterSet (data_p, param_set_p, group_p, FIELD_TRIAL_PARENT_ID.npt_type, FIELD_TRIAL_PARENT_ID.npt_name_s, "Programme", "The Programme that this trial is a part of", programme_id_s, PL_ALL)) != NULL)
				{
					Programme *programme_p = NULL;

					if (programme_id_s && (strcmp (programme_id_s, FT_EMPTY_LIST_OPTION_S) != 0))
						{
							programme_p = GetProgrammeByIdString (programme_id_s, VF_CLIENT_MINIMAL, dfw_data_p);
						}

					param_p -> pa_read_only_flag = read_only_flag;

					if (SetUpProgrammesListParameter (dfw_data_p, (StringParameter *) param_p, programme_p, false))
						{
							if ((param_p = EasyCreateAndAddStringParameterToParameterSet (data_p, param_set_p, group_p, FIELD_TRIAL_TEAM.npt_type, FIELD_TRIAL_TEAM.npt_name_s, "Team", "The team name of the Field Trial", team_s, PL_ALL)) != NULL)
								{
									param_p -> pa_read_only_flag = read_only_flag;

									if (AddMultiplePeopleParameters (param_set_p, "Investigators", existing_people_p, read_only_flag, dfw_data_p))
										{
											success_flag = true;
										}
									else
										{
											PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "AddMultiplePeopleParameters () failed for \"%s\"", name_s ? name_s : "NULL");
										}

								}
							else
								{
									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add %s parameter", FIELD_TRIAL_TEAM.npt_name_s);
								}
						}
				}

		}
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add %s parameter", FIELD_TRIAL_NAME.npt_name_s);
		}

	return success_flag;
}


bool RunForSubmissionFieldTrialParams (FieldTrialServiceData *data_p, ParameterSet *param_set_p, ServiceJob *job_p)
{
	bool job_done_flag = false;
	const char *name_s = NULL;
	const char *id_s = NULL;
	bson_oid_t *trial_id_p = NULL;
	const char *programme_id_s = NULL;
	Programme *programme_p = NULL;
	
	/*
	 * Get the existing study id if specified
	 */
	GetCurrentStringParameterValueFromParameterSet (param_set_p, FIELD_TRIAL_ID.npt_name_s, &id_s);

	if (id_s)
		{
			if (strcmp (FT_EMPTY_LIST_OPTION_S, id_s) != 0)
				{
					trial_id_p = GetBSONOidFromString (id_s);

					if (!trial_id_p)
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to load trial \"%s\" for editing", id_s);
							char *error_s = ConcatenateVarargsStrings ("Failed to load trial with id: '", id_s, "' for editing");

							if (error_s)
								{
									AddParameterErrorMessageToServiceJob (job_p, FIELD_TRIAL_ID.npt_name_s, FIELD_TRIAL_ID.npt_type, error_s);
									FreeCopiedString (error_s);
								}
							else
								{
									AddParameterErrorMessageToServiceJob (job_p, FIELD_TRIAL_ID.npt_name_s, FIELD_TRIAL_ID.npt_type, "Failed to find existing trial");
								}

							return false;
						}
				}
		}		/* if (id_value.st_string_value_s) */


	GetCurrentStringParameterValueFromParameterSet (param_set_p, FIELD_TRIAL_PARENT_ID.npt_name_s, &programme_id_s);

	if (programme_id_s)
		{
			if (strcmp (FT_EMPTY_LIST_OPTION_S, programme_id_s) != 0)
				{
					programme_p = GetProgrammeByIdString (programme_id_s, VF_STORAGE, data_p);

					if (!programme_p)
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to find programme \"%s\" for editing", programme_id_s);
							char *error_s = ConcatenateVarargsStrings ("Failed to find programme with id: '", programme_id_s, "' for editing");

							if (error_s)
								{
									AddParameterErrorMessageToServiceJob (job_p, FIELD_TRIAL_PARENT_ID.npt_name_s, FIELD_TRIAL_PARENT_ID.npt_type, error_s);
									FreeCopiedString (error_s);
								}
							else
								{
									AddParameterErrorMessageToServiceJob (job_p, FIELD_TRIAL_PARENT_ID.npt_name_s, FIELD_TRIAL_PARENT_ID.npt_type, "Failed to find existing trial");
								}

							return false;
						}
				}
		}		/* if (id_value.st_string_value_s) */



	if (GetCurrentStringParameterValueFromParameterSet (param_set_p, FIELD_TRIAL_NAME.npt_name_s, &name_s))
		{
			const char *team_s = NULL;

			if (!IsStringEmpty (name_s))
				{
					FieldTrial *trial_p = NULL;

					GetCurrentStringParameterValueFromParameterSet (param_set_p, FIELD_TRIAL_TEAM.npt_name_s, &team_s);

					trial_p = AllocateFieldTrial (name_s, team_s, programme_p, MF_ALREADY_FREED, trial_id_p, NULL);

					if (trial_p)
						{
							OperationStatus people_status = ProcessPeople (job_p, param_set_p, ProcessPersonForFieldTrial, trial_p, data_p);

							if ((people_status == OS_SUCCEEDED) || (people_status == OS_PARTIALLY_SUCCEEDED))
								{
									if (!SaveFieldTrial (trial_p, job_p, data_p))
										{
											char *error_s = ConcatenateVarargsStrings ("Failed to save trial with name: '", name_s, "' team: '", team_s, "' id: '", id_s, "' programme: '", programme_id_s, "'", NULL);

											if (error_s)
												{
													AddGeneralErrorMessageToServiceJob (job_p, error_s);
													FreeCopiedString (error_s);
												}
											else
												{
													AddGeneralErrorMessageToServiceJob (job_p, "Failed to save Field Trial");
												}

											PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to save trial, name: \"%s\" team: \"%s\", id: \"%s\", programme: \"%s\"", name_s, team_s, id_s, programme_id_s);
										}
								}
							else
								{
									char *error_s = ConcatenateVarargsStrings ("Failed to add people for trial with name: '", name_s, "' team: '", team_s, "' id: '", id_s, "' programme: '", programme_id_s, "'", NULL);

									if (error_s)
										{
											AddGeneralErrorMessageToServiceJob (job_p, error_s);
											FreeCopiedString (error_s);
										}
									else
										{
											AddGeneralErrorMessageToServiceJob (job_p, "Failed to add people for Field Trial");
										}

									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add people for trial, name: \"%s\" team: \"%s\", id: \"%s\", programme: \"%s\"", name_s, team_s, id_s, programme_id_s);

								}

							FreeFieldTrial (trial_p);
						}		/* if (trial_p) */
					else
						{
							char *error_s = ConcatenateVarargsStrings ("Failed to create trial with name: '", name_s, "' team: '", team_s, "' id: '", id_s, "' programme: '", programme_id_s, "'", NULL);

							if (error_s)
								{
									AddGeneralErrorMessageToServiceJob (job_p, error_s);
									FreeCopiedString (error_s);
								}
							else
								{
									AddGeneralErrorMessageToServiceJob (job_p, "Failed to create Field Trial");
								}

							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to create trial, name: \"%s\" team: \"%s\", id: \"%s\", programme: \"%s\"", name_s, team_s, id_s, programme_id_s);
						}

				}		/* if (!IsStringEmpty (name_s)) */
			else
				{
					AddGeneralErrorMessageToServiceJob (job_p, "Name is required");

					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add trial, name: \"%s\" team: \"%s\", id: \"%s\"", name_s, team_s ? team_s : "", id_s);
				}

			job_done_flag = true;
		}		/* if (GetParameterValueFromParameterSet (param_set_p, S_FIELD_TRIAL_NAME.npt_name_s, &value, true)) */

	return job_done_flag;
}


json_t *GetFieldTrialIndexingData (Service *service_p)
{
	FieldTrialServiceData *data_p = (FieldTrialServiceData *) (service_p -> se_data_p);
	json_t *src_trials_p = GetAllFieldTrialsAsJSON (data_p, true);

	if (src_trials_p)
		{
			if (json_is_array (src_trials_p))
				{
					size_t i;
					json_t *src_trial_p;
					size_t num_added = 0;

					json_array_foreach (src_trials_p, i, src_trial_p)
						{
							if (AddDatatype (src_trial_p, DFTD_FIELD_TRIAL))
								{
									++ num_added;
								}


						}		/* json_array_foreach (src_studies_p, i, src_study_p) */

				}		/* if (json_is_array (src_studies_p)) */

			return src_trials_p;
		}		/* if (src_studies_p) */
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "No field trials for \"%s\"", GetServiceName (service_p));
		}

	return NULL;
}



bool GetSubmissionFieldTrialParameterTypeForNamedParameter (const char *param_name_s, ParameterType *pt_p)
{
	bool success_flag = false;


	const NamedParameterType params [] =
		{
			FIELD_TRIAL_NAME,
			FIELD_TRIAL_TEAM,
			FIELD_TRIAL_ID,
			FIELD_TRIAL_ADD,
			FIELD_TRIAL_PARENT_ID,
			NULL
		};

	if (DefaultGetParameterTypeForNamedParameter (param_name_s, pt_p, params))
		{
			success_flag = true;
		}
	else
		{
			success_flag = GetPersonParameterTypeForNamedParameter (param_name_s, pt_p);
		}

	return success_flag;
}


bool AddSearchFieldTrialParams (ServiceData *data_p, ParameterSet *param_set_p)
{
	bool success_flag = false;
	Parameter *param_p = NULL;
	ParameterGroup *group_p = CreateAndAddParameterGroupToParameterSet ("Field Trials", false, data_p, param_set_p);

	if ((param_p = EasyCreateAndAddStringParameterToParameterSet (data_p, param_set_p, group_p, FIELD_TRIAL_ID.npt_type, FIELD_TRIAL_ID.npt_name_s, "Id", "The Id of the Field Trial", NULL, PL_ADVANCED)) != NULL)
		{
			if ((param_p = EasyCreateAndAddStringParameterToParameterSet (data_p, param_set_p, group_p, FIELD_TRIAL_NAME.npt_type, FIELD_TRIAL_NAME.npt_name_s, "Name", "The name of the Field Trial", NULL, PL_ADVANCED)) != NULL)
				{
					if ((param_p = EasyCreateAndAddStringParameterToParameterSet (data_p, param_set_p, group_p, FIELD_TRIAL_TEAM.npt_type, FIELD_TRIAL_TEAM.npt_name_s, "Team", "The team name of the Field Trial", NULL, PL_ADVANCED)) != NULL)
						{
							if ((param_p = EasyCreateAndAddStringParameterToParameterSet (data_p, param_set_p, group_p, FIELD_TRIAL_SEARCH.npt_type, FIELD_TRIAL_SEARCH.npt_name_s, "Search", "Search for matching Field Trials", NULL, PL_ADVANCED)) != NULL)
								{
									if ((param_p = EasyCreateAndAddStringParameterToParameterSet (data_p, param_set_p, group_p, S_FUZZY_SEARCH_FIELD_TRIALS.npt_type, S_FUZZY_SEARCH_FIELD_TRIALS.npt_name_s, "Fuzzy search", "When doing a search, do a fuzzy search", NULL, PL_ADVANCED)) != NULL)
										{
											bool full_data_flag = false;

											if ((param_p = EasyCreateAndAddBooleanParameterToParameterSet (data_p, param_set_p, group_p, S_FULL_DATA.npt_name_s, "Full data", "When doing a search, get the full data", &full_data_flag, PL_ADVANCED)) != NULL)
												{
													success_flag = true;
												}
											else
												{
													PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add %s parameter", S_FULL_DATA.npt_name_s);
												}
										}
									else
										{
											PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add %s parameter", S_FUZZY_SEARCH_FIELD_TRIALS.npt_name_s);
										}
								}
							else
								{
									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add %s parameter", FIELD_TRIAL_SEARCH.npt_name_s);
								}
						}
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add %s parameter", FIELD_TRIAL_TEAM.npt_name_s);
						}
				}
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add %s parameter", FIELD_TRIAL_NAME.npt_name_s);
				}
		}
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add %s parameter", FIELD_TRIAL_ID.npt_name_s);
		}

	return success_flag;
}



bool GetSearchFieldTrialParameterTypeForNamedParameter (const char *param_name_s, ParameterType *pt_p)
{
	const NamedParameterType params [] =
		{
			FIELD_TRIAL_ID,
			FIELD_TRIAL_NAME,
			FIELD_TRIAL_TEAM,
			FIELD_TRIAL_SEARCH,
			S_FUZZY_SEARCH_FIELD_TRIALS,
			S_FULL_DATA,
			NULL
		};

	return DefaultGetParameterTypeForNamedParameter (param_name_s, pt_p, params);
}


bool RunForSearchFieldTrialParams (FieldTrialServiceData *data_p, ParameterSet *param_set_p, ServiceJob *job_p)
{
	const bool *full_data_flag_p = NULL;
	bool job_done_flag = false;
	const char *id_s = NULL;

	GetCurrentBooleanParameterValueFromParameterSet (param_set_p, S_FULL_DATA.npt_name_s, &full_data_flag_p);

	if (GetCurrentStringParameterValueFromParameterSet (param_set_p, FIELD_TRIAL_ID.npt_name_s, &id_s))
		{
			if (!IsStringEmpty (id_s))
				{
					const size_t l = strlen (id_s);
					if (bson_oid_is_valid (id_s, l))
						{
							FieldTrial *trial_p = NULL;
							ViewFormat format = VF_CLIENT_MINIMAL;

							if ((full_data_flag_p != NULL) && (*full_data_flag_p == true))
								{
									format = VF_CLIENT_FULL;
								}

							trial_p = GetFieldTrialByIdString (id_s, format, data_p);

							if (trial_p)
								{
									if (AddFieldTrialToServiceJob (job_p, trial_p, format, data_p))
										{
											job_done_flag = true;
											SetServiceJobStatus (job_p, OS_SUCCEEDED);
										}

									FreeFieldTrial (trial_p);
								}

						}

					job_done_flag = true;
				}

		}		/* if (GetCurrentParameterValueFromParameterSet (param_set_p, FIELD_TRIAL_ID.npt_name_s, &value)) */


	if (!job_done_flag)
		{
			const char *name_s = NULL;
			const char *team_s = NULL;
			const bool *search_flag_p = NULL;

			if (GetCurrentStringParameterValueFromParameterSet (param_set_p, FIELD_TRIAL_NAME.npt_name_s, &name_s))
				{
					GetCurrentStringParameterValueFromParameterSet (param_set_p, FIELD_TRIAL_NAME.npt_name_s, &team_s);
				}		/* if (GetParameterValueFromParameterSet (param_set_p, S_FIELD_TRIAL_NAME.npt_name_s, &value, true)) */


			if (GetCurrentBooleanParameterValueFromParameterSet (param_set_p, FIELD_TRIAL_SEARCH.npt_name_s, &search_flag_p))
				{
					if ((search_flag_p != NULL) && (*search_flag_p == true))
						{
							bool success_flag;
							const bool *fuzzy_search_flag_p = NULL;
							bool fuzzy_search_flag = false;
							ViewFormat vf = VF_CLIENT_MINIMAL;

							GetCurrentBooleanParameterValueFromParameterSet (param_set_p, S_FUZZY_SEARCH_FIELD_TRIALS.npt_name_s, &fuzzy_search_flag_p);

							if ((fuzzy_search_flag_p != NULL) && (*fuzzy_search_flag_p == true))
								{
									fuzzy_search_flag = true;
								}

							if ((full_data_flag_p != NULL) && (*full_data_flag_p == true))
								{
									vf = VF_CLIENT_FULL;
								}

							success_flag = SearchFieldTrials (job_p, name_s, team_s, fuzzy_search_flag, vf, data_p);

							job_done_flag = true;
						}		/* if (value.st_boolean_value) */
				}

		}		/* if (!job_done_flag) */

	return job_done_flag;
}


json_t *GetAllFieldTrialsAsJSON (const FieldTrialServiceData *data_p, const bool full_data_flag)
{
	json_t *results_p = NULL;

	if (SetMongoToolCollection (data_p -> dftsd_mongo_p, data_p -> dftsd_collection_ss [DFTD_FIELD_TRIAL]))
		{
			bson_t *query_p = NULL;
			bson_t *opts_p = NULL;

			if (full_data_flag)
				{
					opts_p =  BCON_NEW ( "sort", "{", FT_NAME_S, BCON_INT32 (1), "}");
				}
			else
				{
					opts_p =  BCON_NEW ("projection", "{", FT_NAME_S, BCON_BOOL (true), "}",
															"sort", "{", FT_NAME_S, BCON_INT32 (1), "}");
				}

			results_p = GetAllMongoResultsAsJSON (data_p -> dftsd_mongo_p, query_p, opts_p);

			if (opts_p)
				{
					bson_destroy (opts_p);
				}

		}		/* if (SetMongoToolCollection (data_p -> dftsd_mongo_p, data_p -> dftsd_collection_ss [DFTD_LOCATION])) */

	return results_p;
}


bool SetUpFieldTrialsListParameter (const FieldTrialServiceData *data_p, Parameter *param_p,  const char *active_trial_id_s, const bool empty_option_flag)
{
	bool success_flag = false;
	json_t *trials_p = GetAllFieldTrialsAsJSON (data_p, false);

	if (trials_p)
		{
			success_flag = SetUpFieldTrialsListParameterFromJSON (data_p, param_p, active_trial_id_s, empty_option_flag, trials_p);
			json_decref (trials_p);
		}

	return success_flag;
}


bool SetUpFieldTrialsListParameterFromJSON (const FieldTrialServiceData *data_p, Parameter *param_p, const char *active_trial_id_s, const bool empty_option_flag, json_t *trials_p)
{
	bool success_flag = false;
	bool value_set_flag = false;

	if (json_is_array (trials_p))
		{
			const size_t num_results = json_array_size (trials_p);

			success_flag = true;

			if (num_results > 0)
				{
					/*
					 * If there's an empty option, add it
					 */
					if (empty_option_flag)
						{
							success_flag = CreateAndAddStringParameterOption (param_p, FT_EMPTY_LIST_OPTION_S, FT_EMPTY_LIST_OPTION_S);
						}


					if (success_flag)
						{
							size_t i = 0;
							const char *param_value_s = GetStringParameterDefaultValue (param_p);
							bson_oid_t *id_p = GetNewUnitialisedBSONOid ();

							if (id_p)
								{
									while ((i < num_results) && success_flag)
										{
											json_t *entry_p = json_array_get (trials_p, i);

											if (GetMongoIdFromJSON (entry_p, id_p))
												{
													char *id_s = GetBSONOidAsString (id_p);

													if (id_s)
														{
															const char *name_s = GetJSONString (entry_p, FT_NAME_S);

															if (name_s)
																{
																	if (param_value_s && (strcmp (param_value_s, id_s) == 0))
																		{
																			value_set_flag = true;
																		}

																	if (!CreateAndAddStringParameterOption (param_p, id_s, name_s))
																		{
																			success_flag = false;
																			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add param option \"%s\": \"%s\"", id_s, name_s);
																		}

																}		/* if (name_s) */
															else
																{
																	success_flag = false;
																	PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, entry_p, "Failed to get \"%s\"", FT_NAME_S);
																}

															FreeBSONOidString (id_s);
														}
													else
														{
															success_flag = false;
															PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, entry_p, "Failed to get Field Trial BSON oid");
														}

												}		/* if (GetMongoIdFromJSON (entry_p, id_p)) */
											else
												{
													success_flag = false;
													PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, entry_p, "GetMongoIdFromJSON () failed");

												}

											if (success_flag)
												{
													++ i;
												}

										}		/* while ((i < num_results) && success_flag) */

									FreeBSONOid (id_p);
								}		/* if (id_p) */

							/*
							 * If the parameter's value isn't on the list, reset it
							 */
							if ((param_value_s != NULL) && (strcmp (param_value_s, FT_EMPTY_LIST_OPTION_S) != 0) && (value_set_flag == false))
								{
									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "param value \"%s\" not on list of existing trials", param_value_s);
								}

						}		/* if (success_flag) */


				}		/* if (num_results > 0) */
			else
				{
					/* nothing to add */
					success_flag = true;
				}

		}		/* if (json_is_array (results_p)) */



	if (success_flag)
		{
			if (active_trial_id_s)
				{
					success_flag = SetStringParameterDefaultValue (param_p, active_trial_id_s);
				}
		}


	return success_flag;
}


json_t *GetFieldTrialAsFrictionlessDataResource (const FieldTrial *trial_p, const FieldTrialServiceData *data_p)
{
	json_t *trial_fd_p = json_object ();

	if (trial_fd_p)
		{
			bool success_flag = false;

			const char * const FD_SCHEMA_URL_S = "https://grassroots.tools/frictionless-data/schemas/field-trials/trial-resource.json";

			if (SetJSONString (trial_fd_p, FD_PROFILE_S, FD_SCHEMA_URL_S))
				{
					char *id_s = GetBSONOidAsString (trial_p -> ft_id_p);

					if (id_s)
						{
							if (SetJSONString (trial_fd_p, FD_ID_S, id_s))
								{
									if (SetJSONString (trial_fd_p, FD_NAME_S, trial_p -> ft_name_s))
										{
											const char * const TEAM_S = "team";

											if (SetNonTrivialString (trial_fd_p, TEAM_S, trial_p -> ft_team_s, false))
												{
													if (trial_p -> ft_parent_p)
														{
															const char * const PROGRAMME_S = "programme";

															if (SetNonTrivialString (trial_fd_p, PROGRAMME_S, trial_p -> ft_parent_p -> pr_name_s, false))
																{
																	const char * const PEOPLE_S = "people";

																	if (AddPeopleAsFrictionlessData (trial_p -> ft_people_p, PEOPLE_S, trial_fd_p, data_p))
																		{
																			success_flag = true;
																		}
																	else
																		{
																			PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, trial_fd_p, "Failed to set \"%s\" for \"%s\"", PEOPLE_S, trial_p -> ft_name_s);
																		}

																}
															else
																{
																	PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, trial_fd_p, "Failed to set \"%s\": \"%s\"", PROGRAMME_S, trial_p -> ft_parent_p -> pr_name_s);
																}
														}
													else
														{
															success_flag = true;
														}

												}		/* if (SetNonTrivialString (trial_fd_p, TEAM_S, trial_p -> ft_team_s, false)) */
											else
												{
													PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, trial_fd_p, "Failed to set \"%s\": \"%s\"", TEAM_S, trial_p -> ft_team_s);
												}

										}		/* if (SetJSONString (trial_fd_p, FD_NAME_S, trial_p -> ft_name_s)) */
									else
										{
											PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, trial_fd_p, "Failed to set \"%s\": \"%s\"", FD_NAME_S, trial_p -> ft_name_s);
										}

								}		/* if (SetJSONString (trial_fd_p, FD_ID_S, id_s)) */

							FreeBSONOidString (id_s);
						}		/* if (id_s) */
					else
						{
							PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, trial_fd_p, "Failed to get id as string for \"%s\"", trial_p -> ft_name_s);
						}

				}		/* if (SetJSONString (trial_fd_p, FD_PROFILE_S, FD_SCHEMA_URL_S)) */
			else
				{
					PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, trial_fd_p, "Failed to set \"%s\": \"%s\"", FD_PROFILE_S, FD_SCHEMA_URL_S);
				}

			if (!success_flag)
				{
					json_decref (trial_fd_p);
					trial_fd_p = NULL;
				}

		}		/* if (trial_fd_p) */
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to create Frictionless Data JSON for \"%s\"", trial_p -> ft_name_s);
		}

	return trial_fd_p;

}



static bool AddFieldTrialToServiceJobResult (ServiceJob *job_p, FieldTrial *trial_p, json_t *trial_json_p, const ViewFormat format, FieldTrialServiceData *data_p)
{
	bool success_flag = false;

	if (GetAllFieldTrialStudies (trial_p, format, data_p))
		{
			if (AddStudiesToFieldTrialJSON (trial_p, trial_json_p, format, data_p))
				{
					success_flag = true;
				}		/* if (AddStudiesToFieldTrialJSON (trial_p, trial_json_p, data_p)) */

		}

	if (success_flag)
		{
			char *title_s = GetFieldTrialAsString (trial_p);

			success_flag = false;

			if (title_s)
				{
					if (AddContext (trial_json_p))
						{
							json_t *dest_record_p = GetDataResourceAsJSONByParts (PROTOCOL_INLINE_S, NULL, title_s, trial_json_p);

							if (dest_record_p)
								{
									AddImage (dest_record_p, DFTD_FIELD_TRIAL, data_p);

									if (AddResultToServiceJob (job_p, dest_record_p))
										{
											success_flag = true;
										}
									else
										{
											json_decref (dest_record_p);
										}

								}		/* if (dest_record_p) */

						}		/* if (AddContext (trial_json_p)) */

					FreeCopiedString (title_s);
				}		/* if (title_s) */

		}

	return success_flag;
}


json_t *GetFieldTrialJSONForId (const char *id_s, const ViewFormat format, JSONProcessor *processor_p, char **name_ss, FieldTrialServiceData *data_p)
{
	json_t *trial_json_p = NULL;
	FieldTrial *trial_p = GetFieldTrialByIdString (id_s, format, data_p);

	if (trial_p)
		{
			if (format == VF_CLIENT_FULL)
				{

				}
			else
				{
					trial_json_p = GetFieldTrialAsJSON (trial_p, format, data_p);

				}

			*name_ss = EasyCopyToNewString (trial_p -> ft_name_s);

			FreeFieldTrial (trial_p);
		}		/* if (trial_p) */

	return trial_json_p;
}


bool AddFieldTrialToServiceJob (ServiceJob *job_p, FieldTrial *trial_p, const ViewFormat format, FieldTrialServiceData *data_p)
{
	bool success_flag = false;
	json_t *trial_json_p = GetFieldTrialAsJSON (trial_p, format, data_p);

	if (trial_json_p)
		{
			if (AddFieldTrialToServiceJobResult (job_p, trial_p, trial_json_p, format, data_p))
				{
					success_flag = true;
				}

			json_decref (trial_json_p);
		}		/* if (trial_json_p) */

	return success_flag;
}


bool AddFieldTrialToServiceJobFromJSON (ServiceJob *job_p, json_t *src_json_p, const ViewFormat format, FieldTrialServiceData *data_p)
{
	bool success_flag = false;
	FieldTrial *trial_p = GetFieldTrialFromJSON (src_json_p, format, data_p);

	if (trial_p)
		{
			json_t *trial_json_p = GetFieldTrialAsJSON (trial_p, format, data_p);

			if (trial_json_p)
				{
					if (AddFieldTrialToServiceJobResult (job_p, trial_p, trial_json_p, format, data_p))
						{
							success_flag = true;
						}

					json_decref (trial_json_p);
				}		/* if (trial_json_p) */
			else
				{
					PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, src_json_p, "GetFieldTrialAsJSON () for format %d failed", format);
				}

			FreeFieldTrial (trial_p);
		}		/* if (trial_p) */
	else
		{
			PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, src_json_p, "GetFieldTrialFromJSON () failed");
		}

	return success_flag;
}





bool SearchFieldTrials (ServiceJob *job_p, const char *name_s, const char *team_s, const bool regex_flag, const ViewFormat format, FieldTrialServiceData *data_p)
{
	bool success_flag = false;
	OperationStatus status = OS_FAILED_TO_START;

	if (SetMongoToolCollection (data_p -> dftsd_mongo_p, data_p -> dftsd_collection_ss [DFTD_FIELD_TRIAL]))
		{
			bson_t *query_p = bson_new ();

			/*
			 * Make the query to get the matching field trials
			 */
			if (query_p)
				{
					bool ok_flag = AddQueryTerm (query_p, FT_NAME_S, name_s, regex_flag);

					if (ok_flag)
						{
							ok_flag = AddQueryTerm (query_p, FT_TEAM_S, team_s, regex_flag);
						}

					if (ok_flag)
						{
							bson_t *opts_p =  BCON_NEW ( "sort", "{", FT_TEAM_S, BCON_INT32 (1), "}");

							if (opts_p)
								{
									json_t *results_p = GetAllMongoResultsAsJSON (data_p -> dftsd_mongo_p, query_p, opts_p);

									if (results_p)
										{
											if (json_is_array (results_p))
												{
													size_t i;
													const size_t num_results = json_array_size (results_p);

													success_flag = true;

													if (num_results > 0)
														{
															json_t *trial_json_p;

															json_array_foreach (results_p, i, trial_json_p)
																{
																	if (!AddFieldTrialToServiceJobFromJSON (job_p, trial_json_p, format, data_p))
																		{
																			PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, trial_json_p, "Failed to add FieldTrial to ServiceJob");
																		}
																}		/* json_array_foreach (results_p, i, entry_p) */

															i = GetNumberOfServiceJobResults (job_p);


														}		/* if (num_results > 0) */

													if (i == num_results)
														{
															status = OS_SUCCEEDED;
														}
													else if (i == 0)
														{
															status = OS_FAILED;
														}
													else
														{
															status = OS_PARTIALLY_SUCCEEDED;
														}


												}		/* if (json_is_array (results_p)) */

											json_decref (results_p);
										}		/* if (results_p) */

									bson_destroy (opts_p);
								}		/* if (opts_p) */
						}

					bson_destroy (query_p);
				}		/* if (query_p) */

		}

	if (!success_flag)
		{
			status = OS_FAILED;
		}

	SetServiceJobStatus (job_p, status);

	return success_flag;
}



static bool SetUpDefaults (char **id_ss, char **program_id_ss, const char **name_ss, const char **team_ss, LinkedList **existing_people_pp)
{
	bool success_flag = true;

	*id_ss = (char *) FT_EMPTY_LIST_OPTION_S;
	*program_id_ss = (char *) FT_EMPTY_LIST_OPTION_S;
	*name_ss = NULL;
	*team_ss = NULL;
	*existing_people_pp = NULL;

	return success_flag;
}



static bool SetUpDefaultsFromExistingFieldTrial (const FieldTrial * const trial_p, char **id_ss, char **program_id_ss, const char **name_ss, const char **team_ss, LinkedList **existing_people_pp)
{
	char *trial_id_s = GetBSONOidAsString (trial_p -> ft_id_p);

	if (trial_id_s)
		{
			char *program_id_s = NULL;

			if (trial_p -> ft_parent_p)
				{
					program_id_s = GetBSONOidAsString (trial_p -> ft_parent_p -> pr_id_p);

					if (!program_id_s)
						{
							FreeBSONOidString (trial_id_s);
							return false;
						}
				}

			*id_ss = trial_id_s;
			*program_id_ss = program_id_s;
			*name_ss = trial_p -> ft_name_s;
			*team_ss = trial_p -> ft_team_s;
			*existing_people_pp = trial_p -> ft_people_p;

			return true;
		}		/* if (trial_id_s) */
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to copy trial id for \"%s\"", trial_p -> ft_name_s);
		}

	return false;
}



FieldTrial *GetFieldTrialFromResource (DataResource *resource_p, const NamedParameterType trial_param_type, FieldTrialServiceData *dfw_data_p)
{
	FieldTrial *trial_p = NULL;

	/*GetFieldTrialFromResourceGetFieldTrialFromResource
	 * Have we been set some parameter values to refresh from?
	 */
	if (resource_p && (resource_p -> re_data_p))
		{
			const json_t *param_set_json_p = json_object_get (resource_p -> re_data_p, PARAM_SET_KEY_S);

			if (param_set_json_p)
				{
					json_t *params_json_p = json_object_get (param_set_json_p, PARAM_SET_PARAMS_S);

					if (params_json_p)
						{
							const char *trial_id_s = GetNamedParameterDefaultValueFromJSON (trial_param_type.npt_name_s, params_json_p);

							/*
							 * Do we have an existing trial id?
							 */
							if (trial_id_s)
								{
									trial_p = GetFieldTrialByIdString (trial_id_s, VF_CLIENT_MINIMAL , dfw_data_p);

									if (!trial_p)
										{
											PrintJSONToErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, params_json_p, "Failed to load Field Trial with id \"%s\"", trial_id_s);
										}

								}		/* if (study_id_s) */

						}
					else
						{
							PrintJSONToErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, param_set_json_p, "Failed to get params with key \"%s\"", PARAM_SET_PARAMS_S);
						}
				}
			else
				{
					PrintJSONToErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, resource_p -> re_data_p, "Failed to get param set with key \"%s\"", PARAM_SET_KEY_S);
				}

		}		/* if (resource_p && (resource_p -> re_data_p)) */

	return trial_p;
}


static bool ProcessPersonForFieldTrial (Person *person_p, void *user_data_p)
{
	FieldTrial *trial_p = (FieldTrial *) user_data_p;

	return AddFieldTrialPerson (trial_p, person_p, MF_SHALLOW_COPY);
}

