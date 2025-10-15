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
 * treatment_factor.c
 *
 *  Created on: 4 Dec 2020
 *      Author: billy
 */



#include "jansson.h"
#include "json_util.h"
#include "key_value_pair.h"
#include "memory_allocations.h"
#include "mongodb_tool.h"
#include "study.h"

#define ALLOCATE_TREATMENT_FACTOR_TAGS (1)
#include "treatment_factor.h"
#include "typedefs.h"
#include "treatment_jobs.h"
#include "treatment_factor_jobs.h"



/*
 * Static Declarations
 */


static bool AddKeyValuePairAsJSON (const KeyValuePair *pair_p, json_t *array_p, const ViewFormat format);



/*
 * API Definitions
 */

TreatmentFactor *AllocateTreatmentFactor (Treatment *treatment_p, Study *study_p)
{
	LinkedList *list_p = AllocateLinkedList (FreeKeyValuePairNode);

	if (list_p)
		{
			TreatmentFactor *tf_p = (TreatmentFactor *) AllocMemory (sizeof (TreatmentFactor));

			if (tf_p)
				{
					tf_p -> tf_study_p = study_p;
					tf_p -> tf_treatment_p = treatment_p;
					tf_p -> tf_values_p = list_p;

					return tf_p;
				}

			FreeLinkedList (list_p);
		}

	return NULL;
}


void FreeTreatmentFactor (TreatmentFactor *treatment_factor_p)
{
	FreeTreatment (treatment_factor_p -> tf_treatment_p);
	FreeLinkedList (treatment_factor_p -> tf_values_p);

	FreeMemory (treatment_factor_p);
}



TreatmentFactor *CopyTreatmentFactor (const TreatmentFactor * const src_p, const Study * const dest_study_p)
{
	Treatment *copied_treatment_p = CopyTreatment (src_p -> tf_treatment_p);

	if (copied_treatment_p)
		{
			TreatmentFactor *dest_p = AllocateTreatmentFactor (copied_treatment_p, dest_study_p);

			if (dest_p)
				{
					bool success_flag = true;
					KeyValuePairNode *src_node_p = (KeyValuePairNode *) (src_p -> tf_values_p -> ll_head_p);

					while (src_node_p && success_flag)
						{
							KeyValuePair *pair_p = src_node_p -> kvpn_pair_p;


							if (AddTreatmentFactorValue (dest_p, pair_p -> kvp_key_s, pair_p -> kvp_value_s))
								{
									src_node_p = (KeyValuePairNode *) (src_node_p -> kvpn_node.ln_next_p);
								}
							else
								{
									success_flag = false;
								}

						}


					if (success_flag)
						{
							return dest_p;
						}
					else
						{
							FreeTreatmentFactor (dest_p);
						}

				}		/* if (dest_p) */
			else
				{
					FreeTreatment (copied_treatment_p);
				}

		}		/* if (copied_treatment_p) */

	return NULL;
}


bool AddTreatmentFactorValue (TreatmentFactor *treatment_factor_p, const char *name_s, const char *value_s)
{
	bool success_flag = false;
	KeyValuePairNode *node_p = AllocateKeyValuePairNodeByParts (name_s, value_s);

	if (node_p)
		{
			LinkedListAddTail (treatment_factor_p -> tf_values_p, & (node_p -> kvpn_node));
			success_flag = true;
		}

	return success_flag;
}


TreatmentFactorNode *AllocateTreatmentFactorNode (TreatmentFactor *treatment_factor_p)
{
	TreatmentFactorNode *node_p = (TreatmentFactorNode *) AllocMemory (sizeof (TreatmentFactorNode));

	if (node_p)
		{
			InitListItem (& (node_p -> tfn_node));
			node_p -> tfn_p = treatment_factor_p;
		}

	return node_p;
}


void FreeTreatmentFactorNode (ListItem *node_p)
{
	TreatmentFactorNode *tf_node_p = (TreatmentFactorNode *) node_p;

	FreeTreatmentFactor (tf_node_p -> tfn_p);

	FreeMemory (node_p);
}


const char *GetTreatmentFactorValue (TreatmentFactor *treatment_p, const char *name_s)
{
	KeyValuePairNode *node_p = (KeyValuePairNode *) (treatment_p -> tf_values_p -> ll_head_p);

	while (node_p)
		{
			const KeyValuePair *pair_p = node_p -> kvpn_pair_p;

			if (strcmp (pair_p -> kvp_key_s, name_s) == 0)
				{
					return (pair_p -> kvp_value_s);
				}
			else
				{
					node_p = (KeyValuePairNode *) (node_p -> kvpn_node.ln_next_p);
				}
		}

	return NULL;
}


json_t *GetTreatmentFactorAsJSON (const TreatmentFactor *treatment_factor_p, const ViewFormat format)
{
	json_t *tf_json_p = json_object ();

	if (tf_json_p)
		{
			bool success_flag = false;

			if (format != VF_INDEXING)
				{
					json_t *values_json_p = GetTreatmentFactorValuesAsJSON (treatment_factor_p, format);

					if (values_json_p)
						{
							if (json_object_set_new (tf_json_p, TF_VALUES_S, values_json_p) == 0)
								{
									success_flag = true;
								}
							else
								{
									json_decref (values_json_p);
								}
						}
				}
			else
				{
					success_flag = true;
				}

			if (success_flag)
				{
					if (AddNamedCompoundIdToJSON (tf_json_p, treatment_factor_p -> tf_study_p -> st_id_p, TF_STUDY_ID_S))
						{
							switch (format)
								{
									case VF_STORAGE:
										{
											if (AddNamedCompoundIdToJSON (tf_json_p, treatment_factor_p -> tf_treatment_p -> tr_id_p, TF_TREATMENT_ID_S))
												{
													success_flag = true;
												}
										}
										break;

									case VF_CLIENT_FULL:
									case VF_CLIENT_MINIMAL:
									case VF_INDEXING:
										{
											json_t *treatment_json_p = GetTreatmentAsJSON (treatment_factor_p -> tf_treatment_p);

											if (treatment_json_p)
												{
													if (json_object_set_new (tf_json_p, TF_TREATMENT_S, treatment_json_p) == 0)
														{
															if (SetJSONString (tf_json_p, TF_STUDY_NAME_S, treatment_factor_p -> tf_study_p -> st_name_s))
																{
																	success_flag = true;
																}
														}
													else
														{
															json_decref (treatment_json_p);
														}
												}

										}
										break;


								}


						}

					if (success_flag)
						{
							return tf_json_p;
						}

				}		/* if (success_flag) */


			json_decref (tf_json_p);
		}		/* if (tf_json_p) */

	return NULL;
}


TreatmentFactor *GetTreatmentFactorFromJSON (const json_t *treatment_factor_json_p, struct Study *parent_study_p, const FieldTrialServiceData *data_p)
{
	TreatmentFactor *tf_p = NULL;
	bson_oid_t *treatment_id_p = GetNewUnitialisedBSONOid ();

	if (treatment_id_p)
		{
			if (GetNamedIdFromJSON (treatment_factor_json_p, TF_TREATMENT_ID_S, treatment_id_p))
				{

					Treatment *treatment_p = GetTreatmentById (treatment_id_p, VF_STORAGE, data_p);

					tf_p = AllocateTreatmentFactor (treatment_p, parent_study_p );

					if (tf_p)
						{
							bool success_flag = false;
							json_t *values_p = json_object_get (treatment_factor_json_p, TF_VALUES_S);

							if (values_p)
								{
									if (json_is_array (values_p))
										{
											const size_t num_values = json_array_size (values_p);
											size_t i;

											for (i = 0; i < num_values; ++ i)
												{
													const json_t *value_p = json_array_get (values_p, i);
													const char *key_s = GetJSONString (value_p, TF_VALUES_KEY_S);

													if (key_s)
														{
															const char *value_s = GetJSONString (value_p, TF_VALUES_VALUE_S);

															if (value_s)
																{
																	if (AddTreatmentFactorValue (tf_p, key_s, value_s))
																		{

																		}		/* if (AddTreatmentFactorValue (tf_p, key_s, value_s)) */

																}		/* if (value_s) */

														}			/* if (key_s) */

												}		/* for (i = 0; i < num_values; ++ i) */

											if (tf_p -> tf_values_p -> ll_size == num_values)
												{
													success_flag = true;
												}

										}		/* if (json_is_array (values_p)) */

								}		/* if (values_p) */
							else
								{
									success_flag = true;
								}

							if (!success_flag)
								{
									FreeTreatmentFactor (tf_p);
									tf_p = NULL;
								}

						}		/* if (tf_p) */

				}		/* if (GetNamedIdFromJSON (treatment_factor_json_p, TF_STUDY_ID_S, treatment_id_p)) */

			FreeBSONOid (treatment_id_p);
		}		/* if (treatment_id_p) */

	return tf_p;
}


const char *GetTreatmentFactorName (const TreatmentFactor *treatment_factor_p)
{
	return treatment_factor_p -> tf_treatment_p -> tr_ontology_term_p -> st_name_s;
}


const char *GetTreatmentFactorUrl (const TreatmentFactor *treatment_factor_p)
{
	return treatment_factor_p -> tf_treatment_p -> tr_ontology_term_p -> st_url_s;
}


const char *GetTreatmentFactorDescription (const TreatmentFactor *treatment_factor_p)
{
	return treatment_factor_p -> tf_treatment_p -> tr_ontology_term_p -> st_description_s;
}






/*
 * Static Definitions
 */

json_t *GetTreatmentFactorValuesAsJSON (const TreatmentFactor *treatment_factor_p, const ViewFormat format)
{
	json_t *values_json_p = json_array ();

	if (values_json_p)
		{
			bool success_flag = true;
			KeyValuePairNode *node_p = (KeyValuePairNode *) (treatment_factor_p -> tf_values_p -> ll_head_p);

			while (node_p && success_flag)
				{
					if (AddKeyValuePairAsJSON (node_p -> kvpn_pair_p, values_json_p, format))
						{
							node_p = (KeyValuePairNode *) (node_p -> kvpn_node.ln_next_p);
						}
					else
						{
							success_flag = false;
						}
				}

			if (success_flag)
				{
					return values_json_p;
				}

			json_decref (values_json_p);
		}		/* if (values_json_p) */

	return NULL;
}


static bool AddKeyValuePairAsJSON (const KeyValuePair *pair_p, json_t *array_p, const ViewFormat format)
{
	json_t *entry_p = json_object ();

	if (entry_p)
		{
			const char *key_s = (format == VF_STORAGE) ? TF_VALUES_KEY_S : TFJ_LABEL_TITLE_S;

			if (SetJSONString (entry_p, key_s, pair_p -> kvp_key_s))
				{
					key_s = (format == VF_STORAGE) ? TF_VALUES_VALUE_S : TFJ_VALUE_TITLE_S;

					if (SetJSONString (entry_p, key_s, pair_p -> kvp_value_s))
						{
							if (json_array_append_new (array_p, entry_p) == 0)
								{
									return true;
								}
						}
				}

			json_decref (entry_p);
		}

	return false;
}
