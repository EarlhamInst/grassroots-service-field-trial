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
 * gene_bank_jobs.c
 *
 *  Created on: 23 Oct 2018
 *      Author: billy
 */


#include "gene_bank_jobs.h"
#include "gene_bank.h"
#include "streams.h"

#include "boolean_parameter.h"
#include "string_parameter.h"

static NamedParameterType S_GENE_BANK_NAME = { "GB Name", PT_STRING };
static NamedParameterType S_GENE_BANK_URL = { "GB Url", PT_STRING};
static NamedParameterType S_GENE_BANK_API_URL = { "GB API Url", PT_STRING };
static NamedParameterType S_ADD_GENE_BANK = { "GB Add Gene Bank", PT_BOOLEAN };


static bool AddGeneBank (ServiceJob *job_p, ParameterSet *param_set_p, FieldTrialServiceData *data_p);


/*
 * API definitions
 */

bool AddSubmissionGeneBankParams (ServiceData *data_p, ParameterSet *param_set_p)
{
	bool success_flag = false;
	ParameterGroup *group_p = CreateAndAddParameterGroupToParameterSet ("Gene Bank", false, data_p, param_set_p);

	if (group_p)
		{
			Parameter *param_p = NULL;

			if ((param_p = EasyCreateAndAddStringParameterToParameterSet (data_p, param_set_p, group_p, S_GENE_BANK_NAME.npt_type, S_GENE_BANK_NAME.npt_name_s, "Name", "The name of the Gene Bank", NULL, PL_ALL)) != NULL)
				{
					param_p -> pa_required_flag = true;

					if ((param_p = EasyCreateAndAddStringParameterToParameterSet (data_p, param_set_p, group_p, S_GENE_BANK_URL.npt_type, S_GENE_BANK_URL.npt_name_s, "URL", "The web page with information about the Gene Bank", NULL, PL_ALL)) != NULL)
						{
							param_p -> pa_required_flag = true;

							if ((param_p = EasyCreateAndAddStringParameterToParameterSet (data_p, param_set_p, group_p, S_GENE_BANK_API_URL.npt_type, S_GENE_BANK_API_URL.npt_name_s, "API URL", "The API URL for the Gene Bank", NULL, PL_ALL)) != NULL)
								{
									bool add_flag = false;

									if ((param_p = EasyCreateAndAddBooleanParameterToParameterSet (data_p, param_set_p, group_p, S_ADD_GENE_BANK.npt_name_s, "Add", "Add Gene Bank", &add_flag, PL_ALL)) != NULL)
										{
											success_flag = true;
										}
									else
										{
											PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add param \"%s\"", S_ADD_GENE_BANK.npt_name_s);
										}

								}
							else
								{
									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add param \"%s\"", S_GENE_BANK_API_URL.npt_name_s);
								}
						}
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add param \"%s\"", S_GENE_BANK_URL.npt_name_s);
						}

				}
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add param \"%s\"", S_GENE_BANK_NAME.npt_name_s);
				}

		}		/* if (group_p) */
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to create Gene Bank Parameter Group");
		}

	return success_flag;
}


bool RunForSubmissionGeneBankParams (FieldTrialServiceData *data_p, ParameterSet *param_set_p, ServiceJob *job_p)
{
	bool job_done_flag = false;
	const bool *add_flag_p = NULL;

	if (GetCurrentBooleanParameterValueFromParameterSet (param_set_p, S_ADD_GENE_BANK.npt_name_s, &add_flag_p))
		{
			if ((add_flag_p != NULL) && (*add_flag_p == true))
				{
					bool success_flag = AddGeneBank (job_p, param_set_p, data_p);

					job_done_flag = true;
				}		/* if (add_flag) */

		}		/* if (GetParameterValueFromParameterSet (param_set_p, S_ADD_LOCATION.npt_name_s, &value, true)) */

	return job_done_flag;
}


bool GetSubmissionGeneBankParameterTypeForNamedParameter (const char *param_name_s, ParameterType *pt_p)
{
	bool success_flag = true;

	if (strcmp (param_name_s, S_GENE_BANK_NAME.npt_name_s) == 0)
		{
			*pt_p = S_GENE_BANK_NAME.npt_type;
		}
	else if (strcmp (param_name_s, S_GENE_BANK_URL.npt_name_s) == 0)
		{
			*pt_p = S_GENE_BANK_URL.npt_type;
		}
	else if (strcmp (param_name_s, S_GENE_BANK_API_URL.npt_name_s) == 0)
		{
			*pt_p = S_GENE_BANK_API_URL.npt_type;
		}
	else if (strcmp (param_name_s, S_ADD_GENE_BANK.npt_name_s) == 0)
		{
			*pt_p = S_ADD_GENE_BANK.npt_type;
		}
	else
		{
			success_flag = false;
		}

	return success_flag;
}



bool SetUpGenBanksListParameter (const FieldTrialServiceData *data_p, StringParameter *param_p)
{
	bool success_flag = false;
	MongoTool *mongo_p = GetConfiguredMongoTool (data_p, NULL);

	if (mongo_p)
		{
			if (SetMongoToolCollection (mongo_p, data_p -> dftsd_collection_ss [DFTD_GENE_BANK]))
				{
					bson_t *query_p = NULL;
					bson_t *opts_p = BCON_NEW ( "sort", "{", CONTEXT_PREFIX_SCHEMA_ORG_S "name", BCON_INT32 (1), "}");
					json_t *results_p = GetAllMongoResultsAsJSON (mongo_p, query_p, opts_p);

					if (results_p)
						{
							if (json_is_array (results_p))
								{
									const size_t num_results = json_array_size (results_p);

									if (num_results > 0)
										{
											size_t i = 0;
											json_t *entry_p = json_array_get (results_p, i);
											GeneBank *gene_bank_p = GetGeneBankFromJSON (entry_p);

											if (gene_bank_p)
												{
													char id_s [MONGO_OID_STRING_BUFFER_SIZE];

													bson_oid_to_string (gene_bank_p -> gb_id_p, id_s);

													if (SetStringParameterCurrentValue (param_p, id_s))
														{
															if (SetStringParameterDefaultValue (param_p, id_s))
																{
																	success_flag = CreateAndAddStringParameterOption (& (param_p -> sp_base_param), id_s, gene_bank_p -> gb_name_s);
																}

														}

													FreeGeneBank (gene_bank_p);
												}		/* if (gene_bank_p) */

											if (success_flag)
												{
													for (++ i; i < num_results; ++ i)
														{
															entry_p = json_array_get (results_p, i);
															gene_bank_p = GetGeneBankFromJSON (entry_p);

															if (gene_bank_p)
																{
																	char id_s [MONGO_OID_STRING_BUFFER_SIZE];

																	bson_oid_to_string (gene_bank_p -> gb_id_p, id_s);

																	success_flag = CreateAndAddStringParameterOption (param_p, id_s, gene_bank_p -> gb_name_s);

																	FreeGeneBank (gene_bank_p);
																}		/* if (gene_bank_p) */

														}		/* for (++ i; i < num_results; ++ i) */

												}		/* if (param_p) */

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

				}		/* if (SetMongoToolCollection (data_p -> dftsd_mongo_p, data_p -> dftsd_collection_ss [DFTD_LOCATION])) */


			FreeMongoTool (mongo_p);
		}		/* if (mongo_p) */
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "GetConfiguredMongoTool () failed");
		}



	return success_flag;
}



static bool AddGeneBank (ServiceJob *job_p, ParameterSet *param_set_p, FieldTrialServiceData *data_p)
{
	bool success_flag = false;
	const char *name_s = NULL;

	if (GetCurrentStringParameterValueFromParameterSet (param_set_p, S_GENE_BANK_NAME.npt_name_s, &name_s))
		{
			const char *url_s = NULL;

			if (GetCurrentStringParameterValueFromParameterSet (param_set_p, S_GENE_BANK_URL.npt_name_s, &url_s))
				{
					const char *api_url_s = NULL;

					if (GetCurrentStringParameterValueFromParameterSet (param_set_p, S_GENE_BANK_API_URL.npt_name_s, &api_url_s))
						{
							GeneBank *gene_bank_p = AllocateGeneBank (NULL, name_s, url_s, api_url_s);

							if (gene_bank_p)
								{
									if (SaveGeneBank (gene_bank_p, data_p))
										{
											success_flag = true;
										}
									else
										{
											PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "SaveGeneBank failed for \"%s\", \"%s\", \"%s\"", name_s, url_s, api_url_s);
										}

									FreeGeneBank (gene_bank_p);
								}		/* if (gene_bank_p) */
							else
								{
									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "AllocateGeneBank failed for \"%s\", \"%s\", \"%s\"", name_s, url_s, api_url_s);
								}

						}		/* if (GetParameterValueFromParameterSet (param_set_p, S_GENE_BANK_API_URL.npt_name_s, &api_url_value, true)) */
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to get \"%s\" parameter value", S_GENE_BANK_API_URL.npt_name_s);
						}

				}		/* if (GetParameterValueFromParameterSet (param_set_p, S_GENE_BANK_URL.npt_name_s, &url_value, true)) */
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to get \"%s\" parameter value", S_GENE_BANK_URL.npt_name_s);
				}

		}		/* if (GetParameterValueFromParameterSet (param_set_p, S_GENE_BANK_NAME.npt_name_s, &name_value, true)) */
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to get \"%s\" parameter value", S_GENE_BANK_NAME.npt_name_s);
		}

	SetServiceJobStatus (job_p, success_flag ? OS_SUCCEEDED : OS_FAILED);

	return success_flag;
}

