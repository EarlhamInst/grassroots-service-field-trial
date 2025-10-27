
#define ALLOCATE_PERSON_JOB_TAGS (1)
#include "person_jobs.h"

#include "string_parameter.h"
#include "string_array_parameter.h"

static bool PopulateValues (LinkedList *existing_people_p, char ***existing_names_sss, char ***existing_emails_sss, char ***existing_roles_sss,
													char ***existing_affiliations_sss, char ***existing_orcids_sss);


bool AddMultiplePeopleParameters (ParameterSet *param_set_p, const char *group_s, LinkedList *existing_people_p, const bool read_only_flag, FieldTrialServiceData *ft_data_p)
{
	bool success_flag = false;
	ServiceData *data_p = & (ft_data_p -> dftsd_base_data);
	ParameterGroup *group_p = CreateAndAddParameterGroupToParameterSet (group_s, true, data_p, param_set_p);

	if (group_p)
		{
			size_t num_people = 0;
			Parameter *param_p = NULL;
			Parameter *name_param_p = NULL;
			
			if (existing_people_p)
				{
					num_people = existing_people_p -> ll_size;
				}

			if (num_people > 1)
				{
					 char **names_ss = NULL;
					 char **emails_ss = NULL;
					 char **roles_ss = NULL;
					 char **affiliations_ss = NULL;
					 char **orcids_ss = NULL;
				
					if (PopulateValues (existing_people_p, &names_ss, &emails_ss, &roles_ss, &affiliations_ss, &orcids_ss))
						{							
							if ((name_param_p = EasyCreateAndAddStringArrayParameterToParameterSet (data_p, param_set_p, group_p, PERSON_NAME.npt_name_s, "Person Name", "The name of the Person", names_ss, num_people, PL_ALL)) != NULL)
								{
									name_param_p -> pa_required_flag = true;
									name_param_p -> pa_read_only_flag = read_only_flag;

									if ((param_p = EasyCreateAndAddStringArrayParameterToParameterSet (data_p, param_set_p, group_p, PERSON_EMAIL.npt_name_s, "Person Email", "The email address of the Person", emails_ss, num_people, PL_ALL)) != NULL)
										{
											param_p -> pa_required_flag = true;
											param_p -> pa_read_only_flag = read_only_flag;

											if ((param_p = EasyCreateAndAddStringArrayParameterToParameterSet (data_p, param_set_p, group_p, PERSON_ROLE.npt_name_s, "Person Role", "The role of the Person", roles_ss, num_people, PL_ALL)) != NULL)
												{
													param_p -> pa_read_only_flag = read_only_flag;

													if ((param_p = EasyCreateAndAddStringArrayParameterToParameterSet (data_p, param_set_p, group_p, PERSON_AFFILIATION.npt_name_s, "Person Affiliation", "The affiliation of the Person", affiliations_ss, num_people, PL_ALL)) != NULL)
														{
															param_p -> pa_read_only_flag = read_only_flag;

															if ((param_p = EasyCreateAndAddStringArrayParameterToParameterSet (data_p, param_set_p, group_p, PERSON_ORCID.npt_name_s, "Person OrCID", "The OrCID of the Person", orcids_ss, num_people, PL_ALL)) != NULL)
																{			
																	param_p -> pa_read_only_flag = read_only_flag;

																	success_flag = true; 
																}
															else
																{
																	PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add %s parameter", PERSON_ORCID.npt_name_s);
																}
														}
													else
														{
															PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add %s parameter", PERSON_AFFILIATION.npt_name_s);
														}
												}
											else
												{
													PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add %s parameter", PERSON_ROLE.npt_name_s);
												}

										}
									else
										{
											PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add %s parameter", PERSON_EMAIL.npt_name_s);
										}
								}
							else
								{
									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add %s parameter", PERSON_NAME.npt_name_s);
								}							
						}		/* if (PopulateValues (existing_people_p, &names_ss, &emails_ss, &roles_ss, &affiliations_ss, &orcids_ss)) */

				}		/* if (num_people > 1) */
			else
				{
					char *name_s = NULL;
					char *email_s = NULL;
					char *role_s = NULL;
					char *affiliation_s = NULL;
					char *orcid_s = NULL;
					
					if (num_people == 1)
						{
							Person *person_p = ((PersonNode *) (existing_people_p -> ll_head_p)) -> pn_person_p;
							
							name_s = person_p -> pe_name_s;
							email_s = person_p -> pe_email_s;
							role_s = person_p -> pe_role_s;
							affiliation_s = person_p -> pe_affiliation_s;
							orcid_s = person_p -> pe_orcid_s;
						}
					
	
					if ((name_param_p = EasyCreateAndAddStringParameterToParameterSet (data_p, param_set_p, group_p, PERSON_NAME.npt_type, PERSON_NAME.npt_name_s, "Person Name", "The name of the Person", name_s, PL_ALL)) != NULL)
						{
							name_param_p -> pa_required_flag = true;

							if ((param_p = EasyCreateAndAddStringParameterToParameterSet (data_p, param_set_p, group_p, PERSON_EMAIL.npt_type, PERSON_EMAIL.npt_name_s, "Person Email", "The email address of the Person", email_s, PL_ALL)) != NULL)
								{
									param_p -> pa_required_flag = true;

									if ((param_p = EasyCreateAndAddStringParameterToParameterSet (data_p, param_set_p, group_p, PERSON_ROLE.npt_type, PERSON_ROLE.npt_name_s, "Person Role", "The role of the Person", role_s, PL_ALL)) != NULL)
										{
											if ((param_p = EasyCreateAndAddStringParameterToParameterSet (data_p, param_set_p, group_p, PERSON_AFFILIATION.npt_type, PERSON_AFFILIATION.npt_name_s, "Person Affiliation", "The affiliation of the Person", affiliation_s, PL_ALL)) != NULL)
												{
													if ((param_p = EasyCreateAndAddStringParameterToParameterSet (data_p, param_set_p, group_p, PERSON_ORCID.npt_type, PERSON_ORCID.npt_name_s, "Person OrCID", "The OrCID of the Person", orcid_s, PL_ALL)) != NULL)
														{			
															success_flag = true; 
														}
													else
														{
															PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add %s parameter", PERSON_ORCID.npt_name_s);
														}
												}
											else
												{
													PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add %s parameter", PERSON_AFFILIATION.npt_name_s);
												}
										}
									else
										{
											PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add %s parameter", PERSON_ROLE.npt_name_s);
										}

								}
							else
								{
									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add %s parameter", PERSON_EMAIL.npt_name_s);
								}
						}
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add %s parameter", PERSON_NAME.npt_name_s);
						}
				}

			if (success_flag)
				{			
					if (!AddRepeatableParameterGroupLabelParam (group_p, name_param_p))
						{

						}

				}
		}



	return success_flag;
}



bool GetPersonParameterTypeForNamedParameter (const char *param_name_s, ParameterType *pt_p)
{
	const NamedParameterType params [] =
	{
		PERSON_NAME,
		PERSON_EMAIL,
		PERSON_ROLE,
		PERSON_AFFILIATION,
		PERSON_ORCID,
		{ NULL }
	};


	return DefaultGetParameterTypeForNamedParameter (param_name_s, pt_p, params);
}


OperationStatus ProcessPeople (ServiceJob *job_p, ParameterSet *param_set_p, bool (*process_person_fn) (Person *person_p, void *user_data_p), void *user_data_p, FieldTrialServiceData *ft_service_data_p)
{
	OperationStatus status = OS_FAILED;
	size_t num_names;
	size_t num_emails;
	size_t num_roles;
	size_t num_affiliations;
	size_t num_orcids;
	const char **names_ss = GetStringArrayValuesForParameter (param_set_p, PERSON_NAME.npt_name_s, &num_names);
	const char **emails_ss = GetStringArrayValuesForParameter (param_set_p, PERSON_EMAIL.npt_name_s, &num_emails);
	const char **roles_ss = GetStringArrayValuesForParameter (param_set_p, PERSON_ROLE.npt_name_s, &num_roles);
	const char **affiliations_ss = GetStringArrayValuesForParameter (param_set_p, PERSON_AFFILIATION.npt_name_s, &num_affiliations);
	const char **orcids_ss = GetStringArrayValuesForParameter (param_set_p, PERSON_ORCID.npt_name_s, &num_orcids);

	if ((num_names == num_emails) && (num_roles == num_affiliations) && (num_orcids == num_names) && (num_names == num_roles))
		{
			if (num_names > 0)
				{
					size_t num_successes = 0;
					size_t i;
					const char **name_ss = names_ss;
					const char **email_ss = emails_ss;
					const char **role_ss = roles_ss;
					const char **affiliation_ss = affiliations_ss;
					const char **orcid_ss = orcids_ss;

					for (i = 0; i < num_names; ++ i, ++ name_ss, ++ email_ss, ++ role_ss, ++ affiliation_ss, ++ orcid_ss)
						{
							/*
								 The front end can send json like
								 {
										"param": "PE Name",
										"current_value": [
											null
										]
									},
									{
										"param": "PE Email",
										"current_value": [
											null
										]
									},
									{
										"param": "PE Role",
										"current_value": [
											null
										]
									},
									{
										"param": "PE Affiliation",
										"current_value": [
											null
										]
									},
									{
										"param": "PE Orcid",
										"current_value": [
											null
										]
									}

								so lets check for various null entries
							 */

							if ((*name_ss) && (*email_ss))
								{
									Person *person_p = AllocatePerson (*name_ss, *email_ss, *role_ss, *affiliation_ss, *orcid_ss);

									if (person_p)
										{
											if (process_person_fn (person_p, user_data_p))
												{
													++ num_successes;
												}
											else
												{
													PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "AddFieldTrialPerson () failed for for \"%s\"", person_p -> pe_name_s, person_p -> pe_email_s);
													FreePerson (person_p);
												}


										}
									else
										{
											PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "AllocatePerson () failed for \"%s\"", *name_ss, *email_ss);
										}

								}
							else if ((*role_ss == NULL) && (*affiliation_ss == NULL) && (*orcid_ss == NULL))
								{
									++ num_successes;
								}
							else
								{
									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Name and email are null but with role \"%s\", affiliation \"%s\", orcid \"%s\"",
															 *role_ss ? *role_ss : "NULL", *affiliation_ss ? *affiliation_ss : "NULL", *orcid_ss ? *orcid_ss : "NULL");
								}


						}

					if (num_successes == num_names)
						{
							status = OS_SUCCEEDED;
						}
					else if (num_successes > 0)
						{
							status = OS_PARTIALLY_SUCCEEDED;
						}


				}		/* if (num_names > 0) */
			else
				{
					status = OS_SUCCEEDED;
				}
		}		/* if (num_mv_entries == num_raw_entries == num_corrected_entries == num_start_dates == num_end_dates == num_notes) */
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "differing array lengths: num_names " SIZET_FMT " num_emails " SIZET_FMT
									" num_roles " SIZET_FMT " num_affiliations " SIZET_FMT " num_orcids " SIZET_FMT,
									num_names, num_emails, num_roles, num_affiliations, num_orcids);
		}
		
	return status;
}


bool AddPeopleToFrictionlessData (LinkedList *people_p, const char * const key_s, json_t *frictionless_json_p)
{
	bool success_flag = false;

	if (people_p -> ll_size > 0)
		{
			json_t *people_json_p = json_array ();

			if (people_json_p)
				{
					PersonNode *node_p = (PersonNode *) (people_p -> ll_head_p);
					bool ok_flag = true;

					while (node_p && ok_flag)
						{
							json_t *person_json_p = GetPersonAsFrictionlessData (node_p -> pn_person_p);

							if (person_json_p)
								{
									if (json_array_append_new (people_json_p, person_json_p) == 0)
										{
											node_p = (PersonNode *) (node_p -> pn_node.ln_next_p);
										}
									else
										{
											ok_flag = false;
											PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, person_json_p, "Failed to add Person json to array");
											json_decref (person_json_p);
										}
								}
							else
								{
									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to get Person json for person \"%s\"", node_p -> pn_person_p -> pe_name_s);
								}
						}		/* while (node_p && success_flag) */

					if (ok_flag)
						{
							if (json_object_set_new (frictionless_json_p, key_s, people_json_p) == 0)
								{
									success_flag = true;
								}
							else
								{
									PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, people_json_p, "Failed to add people json for \"%s\"", key_s);

									json_decref (people_json_p);
								}
						}

				}		/* if (people_json_p) */
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate people json object for \"%s\"", key_s);
				}

		}		/* if (trial_p -> ft_studies_p -> ll_size > 0) */
	else
		{
			/* nothing to add */
			success_flag = true;
		}

	return success_flag;
}


bool AddPeopleToJSON (LinkedList *people_p, const char * const key_s, json_t *json_p, const ViewFormat format, const FieldTrialServiceData *data_p)
{
	bool success_flag = true;

	if (people_p -> ll_size > 0)
		{
			json_t *people_json_p = json_array ();

			if (people_json_p)
				{
					PersonNode *node_p = (PersonNode *) (people_p -> ll_head_p);
					bool ok_flag = true;

					while (node_p && ok_flag)
						{
							json_t *person_json_p = GetPersonAsJSON (node_p -> pn_person_p, format, data_p);

							if (person_json_p)
								{
									if (json_array_append_new (people_json_p, person_json_p) == 0)
										{
											node_p = (PersonNode *) (node_p -> pn_node.ln_next_p);
										}
									else
										{
											ok_flag = false;
											PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, person_json_p, "Failed to add Person json to array");
											json_decref (person_json_p);
										}
								}
							else
								{
									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to get Person json for person \"%s\"", node_p -> pn_person_p -> pe_name_s);
								}
						}		/* while (node_p && success_flag) */

					if (ok_flag)
						{
							if (json_object_set_new (json_p, key_s, people_json_p) == 0)
								{
									success_flag = true;
								}
							else
								{
									PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, people_json_p, "Failed to add people json for \"%s\"", key_s);

									json_decref (people_json_p);
								}
						}

				}		/* if (people_json_p) */
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate people json object for \"%s\"", key_s);
				}

		}		/* if (trial_p -> ft_studies_p -> ll_size > 0) */
	else
		{
			/* nothing to add */
			success_flag = true;
		}

	return success_flag;
}


bool AddPeopleAsFrictionlessData (LinkedList *people_p, const char * const key_s, json_t *json_p, const FieldTrialServiceData *data_p)
{
	return AddPeopleToJSON (people_p, key_s, json_p, VF_CLIENT_MINIMAL, data_p);
}



OperationStatus AddPeopleFromJSON (const json_t *people_json_p, bool (*add_person_fn) (Person *person_p, void *user_data_p, MEM_FLAG *mem_p), void *user_data_p, const FieldTrialServiceData *service_data_p)
{
	OperationStatus status = OS_FAILED;
	
	if (json_is_array (people_json_p))
		{
			size_t i = 0;
			size_t num_people = json_array_size (people_json_p);
			size_t num_added = 0;
			
			for (i = 0; i < num_people; ++ i)
				{
					const json_t *person_json_p = json_array_get (people_json_p, i);
					Person *person_p = GetPersonFromJSON (person_json_p, VF_STORAGE, service_data_p);

					if (person_p)
						{
							MEM_FLAG mf = MF_ALREADY_FREED;
							
							if (add_person_fn (person_p, user_data_p, &mf))
								{
									if ((mf == MF_DEEP_COPY) || (mf == MF_ALREADY_FREED))
										{
											FreePerson (person_p);
										}									
								
									++ num_added;
								}
							else
								{
									PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, person_json_p, "Failed to add person \"%s\"", person_p -> pe_name_s);
									FreePerson (person_p);
								}
								
						}
					else
						{
							PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, person_json_p, "GetPersonFromJSON () failed");
						}
				}

			if (num_added == num_people)
				{
					status = OS_SUCCEEDED;
				}
			else if (num_added > 0)
				{
					status = OS_PARTIALLY_SUCCEEDED;
				}
		}
	else
		{
			PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, people_json_p, "Not an array");
		}

	return status;
}



static bool PopulateValues (LinkedList *existing_people_p, char ***existing_names_sss, char ***existing_emails_sss, char ***existing_roles_sss,
													char ***existing_affiliations_sss, char ***existing_orcids_sss)
{
	const uint32 num_entries = existing_people_p -> ll_size;

	if (num_entries > 0)
		{
			char **existing_names_ss = (char **) AllocMemoryArray (num_entries, sizeof (char *));

			if (existing_names_ss)
				{
					char **existing_emails_ss = (char **) AllocMemoryArray (num_entries, sizeof (char *));

					if (existing_emails_ss)
							{
								char **existing_roles_ss = (char  **) AllocMemoryArray (num_entries, sizeof (char *));

								if (existing_roles_ss)
									{
										char **existing_affiliations_ss = (char **) AllocMemoryArray (num_entries, sizeof (char *));

										if (existing_affiliations_ss)
											{
												char **existing_orcids_ss = (char **) AllocMemoryArray (num_entries, sizeof (char *));

												if (existing_orcids_ss)
													{
														PersonNode *person_node_p = (PersonNode *) (existing_people_p -> ll_head_p);
														char **existing_name_ss = existing_names_ss;
														char **existing_email_ss = existing_emails_ss;
														char **existing_role_ss = existing_roles_ss;
														char **existing_affiliation_ss = existing_affiliations_ss;
														char **existing_orcid_ss = existing_orcids_ss;
														bool success_flag = true;

														while (person_node_p && success_flag)
															{
																Person *person_p = person_node_p -> pn_person_p;
															
																if (CopyAndAddStringValue (person_p -> pe_name_s, existing_name_ss))
																	{
																		if (CopyAndAddStringValue (person_p -> pe_email_s, existing_email_ss))
																			{
																				if (CopyAndAddStringValue (person_p -> pe_role_s, existing_role_ss))
																					{
																						if (CopyAndAddStringValue (person_p -> pe_affiliation_s, existing_affiliation_ss))
																							{
																								if (CopyAndAddStringValue (person_p -> pe_orcid_s, existing_orcid_ss))
																									{
																										person_node_p = (PersonNode *) (person_node_p -> pn_node.ln_next_p);

																										++ existing_name_ss;
																										++ existing_email_ss;
																										++ existing_role_ss;
																										++ existing_affiliation_ss;
																										++ existing_orcid_ss;
																									}
																								else
																									{
																										PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "CopyAndAddStringValue () failed for orcid \"%s\"", person_p -> pe_orcid_s);
																										success_flag = false;
																									}

																							}
																						else
																							{
																								PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "CopyAndAddStringValue () failed for affiliation \"%s\"", person_p -> pe_affiliation_s);
																								success_flag = false;
																							}
																							
																					}
																				else
																					{
																						PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "CopyAndAddStringValue () failed for role \"%s\"", person_p -> pe_role_s);
																						success_flag = false;
																					}

																			}
																		else
																			{
																				PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "CopyAndAddStringValue () failed for email \"%s\"", person_p -> pe_email_s);
																				success_flag = false;
																			}
																	}
																else
																	{
																		PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "CopyAndAddStringValue () failed for name \"%s\"", person_p -> pe_name_s);
																		success_flag = false;
																	}


															}		/* while (obs_node_p && loop_flag) */

														if (success_flag)
															{
																*existing_names_sss = existing_names_ss;
																*existing_emails_sss = existing_emails_ss;
																*existing_roles_sss = existing_roles_ss;
																*existing_affiliations_sss = existing_affiliations_ss;
																*existing_orcids_sss = existing_orcids_ss;
																
																return true;
															}		/* if (success_flag) */

														FreeMemory (existing_orcids_ss);
													}		/* if (existing_phenotype_corrected_p) */
												else
													{
														PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate existing_phenotype_corrected_p");
													}

												FreeMemory (existing_affiliations_ss);
											}		/* if (existing_phenotype_end_dates_p) */
										else
											{
												PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate existing_phenotype_end_dates_p");
											}

										FreeMemory (existing_roles_ss);
									}		/* if (existing_phenotype_start_dates_p) */
								else
									{
										PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate existing_phenotype_start_dates_p");
									}

								FreeMemory (existing_emails_ss);
							}		/* if (existing_phenotype_values_p) */
						else
							{
								PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate existing_emails_ss");
							}

					FreeMemory (existing_names_ss);
				}		/* if (existing_mv_names_p) */
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate existing_mv_names_p");
				}
		}
	else
		{
			return true;
		}
		
	return false;
}

