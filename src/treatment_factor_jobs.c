/*
** Copyright 2014-2020 The Earlham Institute
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
 * treatment_factor_jobs.c
 *
 *  Created on: 7 Dec 2020
 *      Author: billy
 */

#define ALLOCATE_TREATMENT_FACTOR_JOB_CONSTANTS (1)
#include "treatment_factor_jobs.h"

#include "string_parameter.h"
#include "json_parameter.h"

#include "study_jobs.h"
#include "treatment_jobs.h"


#include "frictionless_data_util.h"


static const char * const S_EMPTY_LIST_OPTION_S = "<empty>";



static json_t *GetTableParameterHints (void);

static bool AddStringArraysToJSON (json_t *treatment_factor_json_p, const char *key_s, char **values_ss);




bool AddSubmissionTreatmentFactorParams (ServiceData *data_p, ParameterSet *param_set_p, DataResource *resource_p)
{
	bool success_flag = false;
	Parameter *param_p;
	const char *study_id_s = S_EMPTY_LIST_OPTION_S;
	TreatmentFactor *active_tf_p = NULL;
	FieldTrialServiceData *ft_data_p = (FieldTrialServiceData *) data_p;
	Study *active_study_p = GetStudyFromResource (resource_p, TFJ_STUDY_ID, ft_data_p);

	if ((param_p = EasyCreateAndAddStringParameterToParameterSet (data_p, param_set_p, NULL, TFJ_STUDY_ID.npt_type, TFJ_STUDY_ID.npt_name_s, "Study", "Study to load Treatment Factors for", study_id_s, PL_ALL)) != NULL)
		{
			if (SetUpStudiesListParameter (ft_data_p, param_p, active_study_p, true))
				{
					const char *treatment_id_s = S_EMPTY_LIST_OPTION_S;

					/* We want to update all of the values in the form
					 * when a user selects a study from the list so
					 * we need to make the parameter automatically
					 * refresh the values. So we set the
					 * pa_refresh_service_flag to true.
					 */
					param_p -> pa_refresh_service_flag = true;

					if ((param_p = EasyCreateAndAddStringParameterToParameterSet (data_p, param_set_p, NULL, TFJ_TREATMENT_ID.npt_type, TFJ_TREATMENT_ID.npt_name_s, "Load Treatment", "Edit an existing treatment", treatment_id_s, PL_ALL)) != NULL)
						{
							if (SetUpTreatmentFactorsListParameter (ft_data_p, param_p, active_study_p, active_tf_p, true))
								{
									const char *active_tf_name_s = NULL;

									/*
									 * We want to update all of the values in the form
									 * when a user selects a study from the list so
									 * we need to make the parameter automatically
									 * refresh the values. So we set the
									 * pa_refresh_service_flag to true.
									 */
									param_p -> pa_refresh_service_flag = true;


									if (active_tf_p)
										{
											active_tf_name_s = GetTreatmentFactorName (active_tf_p);
										}

									if ((param_p = EasyCreateAndAddStringParameterToParameterSet (data_p, param_set_p, NULL, TFJ_TREATMENT_NAME.npt_type, TFJ_TREATMENT_NAME.npt_name_s, "Treatment name", "The name of the treatment", active_tf_name_s, PL_ALL)) != NULL)
										{
											json_t *tf_json_p = NULL;


											if (active_tf_p)
												{
													tf_json_p = GetTreatmentFactorAsJSON (active_tf_p, VF_CLIENT_FULL);
												}



											if (GetTreatmentFactorTableParameter (param_set_p, NULL, tf_json_p, ft_data_p))
												{
													success_flag = true;
												}

											if (tf_json_p)
												{
													json_decref (tf_json_p);
												}

										}

								}
						}

				}		/* if (SetUpStudiesListParameter (ft_data_p, (StringParameter *) param_p, NULL, true)) */
		}

	return success_flag;
}


bool IsTreatmentFactorParameter (const char * const param_name_s)
{
	ParameterType pt;

	return GetSubmissionTreatmentFactorParameterTypeForNamedParameter (param_name_s, &pt);
}


bool GetSubmissionTreatmentFactorParameterTypeForNamedParameter (const char *param_name_s, ParameterType *pt_p)
{
	bool success_flag = true;

	if (strcmp (param_name_s, TFJ_STUDY_ID.npt_name_s) == 0)
		{
			*pt_p = TFJ_STUDY_ID.npt_type;
		}
	else if (strcmp (param_name_s, TFJ_TREATMENT_ID.npt_name_s) == 0)
		{
			*pt_p = TFJ_TREATMENT_ID.npt_type;
		}
	else if (strcmp (param_name_s, TFJ_TREATMENT_NAME.npt_name_s) == 0)
		{
			*pt_p = TFJ_TREATMENT_NAME.npt_type;
		}
	else if (strcmp (param_name_s, TFJ_VALUES.npt_name_s) == 0)
		{
			*pt_p = TFJ_VALUES.npt_type;
		}
	else
		{
			success_flag = false;
		}

	return success_flag;
}


bool AddTreatmentFactorByPartsToStudy (const char *treatment_url_s, const json_t *factors_json_p, Study *study_p, const FieldTrialServiceData *data_p)
{
	bool success_flag = false;
	size_t num_values = 0;

	if ((json_is_array (factors_json_p)) && ((num_values = json_array_size (factors_json_p)) > 0))
		{
			/* strip any leading or trailing whitespace from the treatment url */
			char *stripped_url_s = CopyToNewString (treatment_url_s, 0, true);

			if (stripped_url_s)
				{
					Treatment *treatment_p = GetTreatmentByURL (stripped_url_s, VF_STORAGE, data_p);

					if (treatment_p)
						{
							TreatmentFactor *tf_p = GetOrCreateTreatmentFactorForStudy (study_p, treatment_p -> tr_id_p, data_p);

							if (tf_p)
								{
									size_t i = 0;

									success_flag = true;

									while ((i < num_values) && success_flag)
										{
											const json_t *factor_json_p = json_array_get (factors_json_p, i);

											if (json_object_size (factor_json_p) > 0)
												{
													const char *name_s = GetJSONString (factor_json_p, TFJ_LABEL_TITLE_S);

														if (name_s)
															{
																const char *value_s = GetJSONString (factor_json_p, TFJ_VALUE_TITLE_S);

																if (value_s)
																	{
																		if (AddTreatmentFactorValue (tf_p, name_s, value_s))
																			{
																				++ i;
																			}
																		else
																			{
																				PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add factor \"%s\": \"s\" to treatment \"%s\" in study \"%s\"", name_s, value_s, GetTreatmentFactorName (tf_p), study_p -> st_name_s);
																				success_flag = false;
																			}		/* if (AddTreatmentFactorValue (tf_p, name_s, value_s)) */

																	}		/* if (value_s) */
																else
																	{
																		PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, factor_json_p, "Failed to get \"%s\"", TFJ_VALUE_TITLE_S);
																		success_flag = false;
																	}

															}		/* if (name_s) */
														else
															{
																PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, factor_json_p, "Failed to get \"%s\"", TFJ_LABEL_TITLE_S);
																success_flag = false;
															}
												}
											else
												{
													++ i;
												}

										}		/* while ((i < num_values) && success_flag) */

									if (i != num_values)
										{
											success_flag = false;
										}

								}		/* if (tf_p) */
							else
								{
									FreeTreatment (treatment_p);
								}

						}		/* if (treatment_p) */
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to get Treatment for url \"%s\"", stripped_url_s);
						}

					FreeCopiedString (stripped_url_s);
				}		/* if (stripped_url_s) */

		}		/* if ((json_is_array (factors_json_p)) && ((num_values = json_array_size (factors_json_p)) > 0)) */

	return success_flag;
}


bool RunForSubmissionTreatmentFactorParams (FieldTrialServiceData *data_p, ParameterSet *param_set_p, ServiceJob *job_p)
{
	bool job_done_flag = false;
	const char *study_id_s = NULL;
	OperationStatus status = OS_IDLE;

	if (GetCurrentStringParameterValueFromParameterSet (param_set_p, TFJ_STUDY_ID.npt_name_s, &study_id_s))
		{
			if (!IsStringEmpty (study_id_s))
				{
					Study *study_p = GetStudyByIdString (study_id_s, VF_CLIENT_MINIMAL, data_p);

					if (study_p)
						{
							const char *tf_url_s = NULL;

							if (GetCurrentStringParameterValueFromParameterSet (param_set_p, TFJ_TREATMENT_NAME.npt_name_s, &tf_url_s))
								{
									Treatment *treatment_p = GetTreatmentByURL (tf_url_s, VF_STORAGE, data_p);

									if (treatment_p)
										{
											const json_t *factors_json_p = NULL;

											if (GetCurrentJSONParameterValueFromParameterSet (param_set_p, TFJ_VALUES.npt_name_s, &factors_json_p))
												{
													status = OS_FAILED;

													if (AddTreatmentFactorByPartsToStudy (tf_url_s, factors_json_p, study_p, data_p))
														{
															if (SaveStudy (study_p, job_p, data_p, NULL))
																{
																	status = OS_SUCCEEDED;
																}
															else
																{
																	PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "SaveStudy failed for url \"%s\" on study \"%s\"", tf_url_s, study_p -> st_name_s);
																}
														}
													else
														{
															PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "AddTreatmentFactorToStudy failed for url \"%s\" on study \"%s\"", tf_url_s, study_p -> st_name_s);
														}

													SetServiceJobStatus (job_p, status);
													job_done_flag = true;
												}		/* if (GetCurrentJSONParameterValueFromParameterSet (param_set_p, TFJ_VALUES.npt_name_s, &factors_json_p)) */

										}		/* if (treatment_p) */
									else
										{
											PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to get Treatment for url \"%s\"", tf_url_s);
										}

									job_done_flag = true;

								}		/* if (GetCurrentStringParameterValueFromParameterSet (param_set_p, TFJ_TREATMENT_NAME.npt_name_s, &tf_url_s)) */

							FreeStudy (study_p);
						}		/* if (study_p) */
					else
						{
							PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Failed to get parent study for plots with id \"%s\"", study_id_s);
						}

				}		/* if (!IsStringEmpty (study_id_s)) */

		}		/* if (GetCurrentParameterValueFromParameterSet (param_set_p, S_STUDIES_LIST.npt_name_s, &parent_study_value)) */
	else
		{
			PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Failed to get param \"%s\"", TFJ_STUDY_ID.npt_name_s);
		}

	return job_done_flag;
}




bool SetUpTreatmentFactorsListParameter (const FieldTrialServiceData *data_p, Parameter *param_p, const Study *active_study_p, const TreatmentFactor *active_tf_p, const bool empty_option_flag)
{
	bool success_flag = false;
	bool value_set_flag = false;

	/*
	 * If there's an empty option, add it
	 */
	if (empty_option_flag)
		{
			if (!CreateAndAddStringParameterOption (param_p, S_EMPTY_LIST_OPTION_S, S_EMPTY_LIST_OPTION_S))
				{
					return false;
				}
		}



	if (active_study_p && (active_study_p -> st_treatments_p))
		{
			bool loop_flag = true;
			TreatmentFactorNode *node_p = (TreatmentFactorNode *) (active_study_p -> st_treatments_p -> ll_head_p);
			size_t num_added = 0;


			while (node_p && loop_flag)
				{
					TreatmentFactor *tf_p = node_p -> tfn_p;
					const char *name_s = GetTreatmentFactorName (tf_p);
					char *id_s = GetBSONOidAsString (tf_p -> tf_treatment_p -> tr_id_p);

					if (id_s)
						{
							if (CreateAndAddStringParameterOption (param_p, id_s, name_s))
								{
									++ num_added;
								}
							else
								{
									loop_flag = false;
								}

							FreeBSONOidString (id_s);
						}

					if (loop_flag)
						{
							node_p = (TreatmentFactorNode *) (node_p -> tfn_node.ln_next_p);
						}

				}		/* while (node_p && loop_flag) */

			if (num_added == active_study_p -> st_treatments_p -> ll_size)
				{
					success_flag = true;
				}
		}
	else
		{
			/* nothing to add to the list */
			success_flag = true;
		}

	return success_flag;
}



Parameter *GetTreatmentFactorTableParameter (ParameterSet *param_set_p, ParameterGroup *group_p, json_t *tf_json_p, const FieldTrialServiceData *data_p)
{
	Parameter *param_p = NULL;
	bool success_flag = false;
	json_t *hints_p = GetTableParameterHints ();

	if (hints_p)
		{
			param_p = EasyCreateAndAddJSONParameterToParameterSet (& (data_p -> dftsd_base_data), param_set_p, group_p, TFJ_VALUES.npt_type, TFJ_VALUES.npt_name_s, "Treatment Factors to upload", "The data to upload", tf_json_p, PL_ALL);

			if (param_p)
				{
					success_flag = false;

					if (AddParameterKeyJSONValuePair (param_p, PA_TABLE_COLUMN_HEADINGS_S, hints_p))
						{
							success_flag = true;
						}

					if (!success_flag)
						{
							FreeParameter (param_p);
							param_p = NULL;
						}

				}		/* if (param_p) */

			json_decref (hints_p);
		}		/* if (hints_p) */


	return param_p;
}



json_t *GetTreatmentFactorAsFrictionlessData (const TreatmentFactor *treatment_factor_p)
{
	json_t *treatment_fd_p = json_object ();

	if (treatment_fd_p)
		{
			bool success_flag = false;

			const char * const SCHEMA_URL_S = "https://grassroots.tools/frictionless-data/schemas/field-trials/treatment-resource.json";

			if (SetJSONString (treatment_fd_p, FD_PROFILE_S, SCHEMA_URL_S))
				{
					char *id_s = GetBSONOidAsString (treatment_factor_p -> tf_treatment_p -> tr_id_p);

					if (id_s)
						{
							if (SetJSONString (treatment_fd_p, FD_ID_S, id_s))
								{
									const char *value_s = GetTreatmentFactorName (treatment_factor_p);

									if (SetJSONString (treatment_fd_p, FD_NAME_S, value_s))
										{
											const char *KEY_S = "url";
											value_s = GetTreatmentFactorUrl (treatment_factor_p);

											if (SetJSONString (treatment_fd_p, KEY_S, value_s))
												{
													value_s = GetTreatmentFactorDescription (treatment_factor_p);

													if (SetJSONString (treatment_fd_p, FD_DESCRIPTION_S, value_s))
														{
															KEY_S = "synonyms";

															if (AddStringArraysToJSON (treatment_fd_p, KEY_S, treatment_factor_p -> tf_treatment_p -> tr_synonyms_ss))
																{
																	KEY_S = "parents";

																	if (AddStringArraysToJSON (treatment_fd_p, KEY_S, treatment_factor_p -> tf_treatment_p -> tr_parent_names_ss))
																		{
																			success_flag = true;

																		}

																}


														}		/* if (SetJSONString (treatment_fd_p, FD_DESCRIPTION_S, value_s)) */
													else
														{

														}

												}		/* if (SetJSONString (treatment_fd_p, KEY_S, value_s)) */
											else
												{

												}


										}		/* if (SetJSONString (treatment_fd_p, FD_NAME_S, study_p -> st_name_s)) */
									else
										{
											PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, treatment_fd_p, "Failed to set \"%s\": \"%s\"", FD_NAME_S, value_s);
										}



								}		/* if (SetJSONString (treatment_fd_p, FD_ID_S, id_s)) */

							FreeBSONOidString (id_s);
						}		/* if (id_s) */
					else
						{

						}

				}		/* if (SetJSONString (treatment_fd_p, FD_PROFILE_S, SCHEMA_URL_S)) */
			else
				{
					PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, treatment_fd_p, "Failed to set \"%s\": \"%s\"", FD_PROFILE_S, SCHEMA_URL_S);
				}


			if (!success_flag)
				{
					json_decref (treatment_fd_p);
					treatment_fd_p = NULL;
				}
		}		/* if (treatment_fd_p) */

	return treatment_fd_p;

}



static json_t *GetTableParameterHints (void)
{
	json_t *hints_p = json_array ();

	if (hints_p)
		{
			if (AddColumnParameterHint (TFJ_LABEL_TITLE_S, "The label to use for the Treatment Factor level", PT_STRING, false, hints_p))
				{
					if (AddColumnParameterHint (TFJ_VALUE_TITLE_S, "The value or description for the Treatment Factor level", PT_STRING, false, hints_p))
						{
							return hints_p;
						}
				}

			json_decref (hints_p);
		}

	return NULL;
}


static bool AddStringArraysToJSON (json_t *treatment_factor_json_p, const char *key_s, char **values_ss)
{
	bool success_flag = false;

	if (values_ss && (*values_ss))
		{
			json_t *array_p = json_array ();

			if (array_p)
				{
					success_flag = true;

					while ((*values_ss) && success_flag)
						{
							json_t *str_p = json_string (*values_ss);

							if (str_p)
								{
									if (json_array_append_new (array_p, str_p) == 0)
										{
											++ values_ss;
										}
									else
										{
											success_flag = false;
										}
								}
							else
								{
									success_flag = false;
								}

						}		/* while ((*values_ss) && success_flag) */

					if (success_flag)
						{
							if (json_object_set_new (treatment_factor_json_p, key_s, array_p) != 0)
								{
									success_flag = false;
								}
						}

				}
		}
	else
		{
			success_flag = true;
		}

	return success_flag;
}
