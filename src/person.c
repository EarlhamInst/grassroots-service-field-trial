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
 * person.c
 *
 *  Created on: 7 Jan 2019
 *      Author: billy
 */

#include "person.h"
#include "memory_allocations.h"
#include "dfw_util.h"


Person *AllocatePerson (const char *name_s, const char *email_s, const char *role_s, const char *affiliation_s, const char *orcid_s)
{
	char *copied_name_s = NULL;

	if (CloneValidString (name_s, &copied_name_s))
		{
			char *copied_email_s = NULL;

			if (CloneValidString (email_s, &copied_email_s))
				{
					char *copied_role_s = NULL;

					if (CloneValidString (role_s, &copied_role_s))
						{
							char *copied_affiliation_s = NULL;

							if (CloneValidString (affiliation_s, &copied_affiliation_s))
								{
									char *copied_orcid_s = NULL;

									if (CloneValidString (orcid_s, &copied_orcid_s))
										{
											Person *person_p = (Person *) AllocMemory (sizeof (Person));

											if (person_p)
												{
													person_p -> pe_name_s = copied_name_s;
													person_p -> pe_email_s = copied_email_s;
													person_p -> pe_role_s = copied_role_s;
													person_p -> pe_affiliation_s = copied_affiliation_s;
													person_p -> pe_orcid_s = copied_orcid_s;

													return person_p;
												}

											if (copied_orcid_s)
												{
													FreeCopiedString (copied_orcid_s);
												}

										}		/* if (CloneValidString (orcid_s, &copied_orcid_s)) */
									else
										{
											PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to copy orcid \"%s\"", orcid_s);
										}

									if (copied_affiliation_s)
										{
											FreeCopiedString (copied_affiliation_s);
										}

								}		/* if (CloneValidString (affiliation_s, &copied_affiliation_s)) */
							else
								{
									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to copy affiliation \"%s\"", affiliation_s);
								}

							if (copied_role_s)
								{
									FreeCopiedString (copied_role_s);
								}

						}		/* if (CloneValidString (role_s, &copied_role_s)) */
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to copy role \"%s\"", role_s);
						}

					if (copied_email_s)
						{
							FreeCopiedString (copied_email_s);
						}

				}		/* if (CloneValidString (email_s, &copied_email_s)) */
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to copy email \"%s\"", email_s);
				}

			if (copied_name_s)
				{
					FreeCopiedString (copied_name_s);
				}
		}
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to copy name \"%s\"", name_s);
		}

	return NULL;
}


void InitPerson (Person *person_p)
{
	person_p -> pe_name_s = NULL;
	person_p -> pe_email_s = NULL;
	person_p -> pe_role_s = NULL;
	person_p -> pe_affiliation_s = NULL;
	person_p -> pe_orcid_s = NULL;
}


void ClearPerson (Person *person_p)
{
	if (person_p -> pe_name_s)
		{
			FreeCopiedString (person_p -> pe_name_s);
		}

	if (person_p -> pe_email_s)
		{
			FreeCopiedString (person_p -> pe_email_s);
		}

	if (person_p -> pe_role_s)
		{
			FreeCopiedString (person_p -> pe_role_s);
		}

	if (person_p -> pe_affiliation_s)
		{
			FreeCopiedString (person_p -> pe_affiliation_s);
		}

	if (person_p -> pe_orcid_s)
		{
			FreeCopiedString (person_p -> pe_orcid_s);
		}
}


void FreePerson (Person *person_p)
{
	ClearPerson (person_p);
	FreeMemory (person_p);
}


Person *CopyPerson (const Person * const src_p)
{
	return AllocatePerson (src_p -> pe_name_s, src_p -> pe_email_s, src_p -> pe_role_s, src_p -> pe_affiliation_s, src_p -> pe_orcid_s);
}



int ComparePeople (const Person *person_0_p, const Person *person_1_p)
{
	int res = strcmp (person_0_p -> pe_name_s, person_1_p -> pe_name_s);

	if (res != 0)
		{
			res = strcmp (person_0_p -> pe_email_s, person_1_p -> pe_email_s);
		}

	return res;
}


bool AddPersonToCompoundJSON (const Person *person_p, json_t *parent_json_p, const char * const key_s,  const ViewFormat format, const FieldTrialServiceData *data_p)
{
	bool success_flag = false;

	json_t *person_json_p = GetPersonAsJSON (person_p, format, data_p);

	if (person_json_p)
		{
			if (json_object_set_new (parent_json_p, key_s, person_json_p) == 0)
				{
					success_flag = true;
				}
			else
				{
					json_decref (parent_json_p);
					PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, parent_json_p, "Failed to add person to json \"%s\" \"%s\"", person_p -> pe_name_s, person_p -> pe_email_s);
				}
		}
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to get person as json \"%s\" \"%s\"", person_p -> pe_name_s, person_p -> pe_email_s);
		}

	return success_flag;
}


Person *GetPersonFromCompoundJSON (const json_t *parent_p, const char * const key_s, const ViewFormat format, const FieldTrialServiceData *data_p)
{
	Person *person_p = NULL;
	const json_t *child_p = json_object_get (parent_p, key_s);

	if (child_p)
		{
			person_p = GetPersonFromJSON (child_p, format, data_p);
		}

	return person_p;
}



json_t *GetPersonAsJSON (const Person * const person_p, const ViewFormat format, const FieldTrialServiceData *data_p)
{
	json_t *person_json_p = json_object ();

	if (person_json_p)
		{
			if (SetNonTrivialString (person_json_p, PE_NAME_S, person_p -> pe_name_s, true))
				{
					if (SetNonTrivialString (person_json_p, PE_EMAIL_S, person_p -> pe_email_s, true))
						{
							if (SetNonTrivialString (person_json_p, PE_ROLE_S, person_p -> pe_role_s, true))
								{
									if (SetNonTrivialString (person_json_p, PE_AFFILATION_S, person_p -> pe_affiliation_s, true))
										{
											if (SetNonTrivialString (person_json_p, PE_ORCID_S, person_p -> pe_orcid_s, true))
												{
													if (SetJSONString (person_json_p, INDEXING_TYPE_S, CONTEXT_PREFIX_SCHEMA_ORG_S "Person"))
														{
															return person_json_p;
														}
													else
														{
															PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, person_json_p, "Failed to add \"%s\": \"%s\"", INDEXING_TYPE_S, CONTEXT_PREFIX_SCHEMA_ORG_S "Person");
														}
												}
											else
												{
													PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, person_json_p, "Failed to add \"%s\": \"%s\"", PE_ORCID_S, person_p -> pe_orcid_s);
												}
										}
									else
										{
											PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, person_json_p, "Failed to add \"%s\": \"%s\"", PE_AFFILATION_S, person_p -> pe_affiliation_s);
										}
								}
							else
								{
									PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, person_json_p, "Failed to add \"%s\": \"%s\"", PE_ROLE_S, person_p -> pe_role_s);
								}
						}
					else
						{
							PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, person_json_p, "Failed to add \"%s\": \"%s\"", PE_EMAIL_S, person_p -> pe_email_s);
						}
				}
			else
				{
					PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, person_json_p, "Failed to add \"%s\": \"%s\"", PE_NAME_S, person_p -> pe_name_s);
				}

			json_decref (person_json_p);
		}

	return NULL;
}


json_t *GetPersonAsFrictionlessData (const Person * const person_p)
{
	json_t *person_json_p = json_object ();

	if (person_json_p)
		{
			const char * const name_key_s = "name";

			if (SetNonTrivialString (person_json_p, name_key_s, person_p -> pe_name_s, true))
				{
					const char * const email_key_s = "email";

					if (SetNonTrivialString (person_json_p, email_key_s, person_p -> pe_email_s, true))
						{
							const char * const role_key_s = "role";

							if (SetNonTrivialString (person_json_p, role_key_s, person_p -> pe_role_s, true))
								{
									const char * const affiliation_key_s = "affiliation";

									if (SetNonTrivialString (person_json_p, affiliation_key_s, person_p -> pe_affiliation_s, true))
										{
											if (SetNonTrivialString (person_json_p, PE_ORCID_S, person_p -> pe_orcid_s, true))
												{
													return person_json_p;
												}
											else
												{
													PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, person_json_p, "Failed to add \"%s\": \"%s\"", PE_ORCID_S, person_p -> pe_orcid_s);
												}
										}
									else
										{
											PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, person_json_p, "Failed to add \"%s\": \"%s\"", affiliation_key_s, person_p -> pe_affiliation_s);
										}
								}
							else
								{
									PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, person_json_p, "Failed to add \"%s\": \"%s\"", role_key_s, person_p -> pe_role_s);
								}
						}
					else
						{
							PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, person_json_p, "Failed to add \"%s\": \"%s\"", email_key_s, person_p -> pe_email_s);
						}
				}
			else
				{
					PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, person_json_p, "Failed to add \"%s\": \"%s\"", name_key_s, person_p -> pe_name_s);
				}

			json_decref (person_json_p);
		}

	return NULL;
}



Person *GetPersonFromJSON (const json_t *json_p, const ViewFormat format, const FieldTrialServiceData *data_p)
{
	const char *name_s = GetJSONString (json_p, PE_NAME_S);
	const char *email_s = GetJSONString (json_p, PE_EMAIL_S);
	const char *role_s = GetJSONString (json_p, PE_ROLE_S);
	const char *affiliation_s = GetJSONString (json_p, PE_AFFILATION_S);
	const char *orcid_s = GetJSONString (json_p, PE_ORCID_S);

	Person *person_p = AllocatePerson (name_s, email_s, role_s, affiliation_s, orcid_s);

	return person_p;
}


PersonNode *AllocatePersonNode (Person *person_p)
{
	PersonNode *node_p = (PersonNode *) AllocMemory (sizeof (PersonNode));
	
	if (node_p)
		{
			InitListItem (& (node_p -> pn_node));
			node_p -> pn_person_p = person_p;
			
			return node_p;
		}
	else
		{
		
		}
		
	return NULL;
}


void FreePersonNode (ListItem *node_p)
{
	PersonNode *person_node_p = (PersonNode *) node_p;
	
	FreePerson (person_node_p -> pn_person_p);
	FreeMemory (person_node_p);
}


