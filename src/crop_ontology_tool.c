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
 * crop_ontology_tool.c
 *
 *  Created on: 25 Oct 2018
 *      Author: billy
 */


/*
 * http://www.cropontology.org/get-attributes/CO_321:0000007
 */
/*
[
  {
    "key": "name",
    "value": "{\"english\":\"Heading time\"}"
  },
  {
    "key": "Trait description",
    "value": "{\"english\":\"Heading time extends from the time of emergence of the tip of the spike from the flag leaf sheath to when the spike has completely emerged but has not yet started to flower.\"}"
  },
  {
    "key": "Trait class",
    "value": "{\"english\":\"Phenological traits\"}"
  },
  {
    "key": "created_at",
    "value": "Fri Dec 02 08:20:19 UTC 2016"
  },
  {
    "key": "ontology_id",
    "value": "CO_321"
  },
  {
    "key": "ontology_name",
    "value": "Wheat"
  },
  {
    "key": "Main trait abbreviation",
    "value": "{\"english\":\"Hd\"}"
  },
  {
    "key": "Trait name",
    "value": "{\"english\":\"Heading time\"}"
  },
  {
    "key": "Alternative trait abbreviations",
    "value": "{\"english\":\"Head, Heading, H\"}"
  },
  {
    "key": "ibfieldbook",
    "value": "{\"english\":\"Obsolete/legacy\"}"
  },
  {
    "key": "Attribute",
    "value": "{\"english\":\"Heading time\"}"
  },
  {
    "key": "language",
    "value": "EN"
  },
  {
    "key": "Entity",
    "value": "{\"english\":\"Plant\"}"
  }
]
*/

#include <string.h>

#define ALLOCATE_CROP_ONTOLOGY_CONSTANTS (1)
#include "crop_ontology_tool.h"
#include "measured_variable.h"
#include "curl_tools.h"
#include "jansson.h"
#include "string_utils.h"
#include "streams.h"
#include "schema_term.h"
#include "json_util.h"
#include "nominal_scale_class.h"


typedef struct
{
	FieldTrialServiceData *cotd_service_data_p;
	CurlTool *cotd_curl_p;
	json_t *cotd_query_p;
	char *cotd_query_key_s;
} COToolData;




/*
 * static declarations
 */


static const char * const S_CROP_ONTOLOGY_API_URL_S = "http://www.cropontology.org/get-attributes/";


static const char * const S_SCALE_CLASS_NAME_S = CONTEXT_PREFIX_SCHEMA_ORG_S "name";
static const char * const S_SCALE_CLASS_TYPE_S = "class_type";




static char *GetTermEnglishValue (const json_t *entry_p);


static SchemaTerm *GetSchemaTerm (json_t *values_p, const char *term_s, const char *name_key_s, const char *abbreviation_key_s, const char *description_key_s, const bool is_unit_term_flag);

static SchemaTerm *GetCachedCropOnotologySchemaTerm (const char *term_s, MongoTool *tool_p);

static SchemaTerm *FindCachedCropOnotologySchemaTerm (const char *term_s, const char *key_stem_s, MongoTool *tool_p, bson_t *opts_p);

static const ScaleClass *GetScaleDatatype (const json_t *document_p);

static const ScaleClass *GetScaleClass (const char *variable_url_s, CurlTool *curl_p);

static bool UpdateScaleTerms (const bson_t *document_p, void *data_p);


/*
 * API definitions
 */



SchemaTerm *GetCropOnotologySchemaTerm (const char *crop_ontology_term_s, TermType expected_type, TermType *found_type_p, MongoTool *mongo_p)
{
	SchemaTerm *term_p = NULL;
	const size_t TERMS_URL_LENGTH = strlen (CONTEXT_URL_CROP_ONTOLOGY_S);
	const char *term_s = NULL;

	if (strncmp (crop_ontology_term_s, CONTEXT_URL_CROP_ONTOLOGY_S, TERMS_URL_LENGTH) == 0)
		{
			term_s = crop_ontology_term_s + TERMS_URL_LENGTH;
		}
	else
		{
			term_s = crop_ontology_term_s;
		}

	if (mongo_p)
		{
			term_p = GetCachedCropOnotologySchemaTerm (term_s, mongo_p);
		}

	if (!term_p)
		{
			char *url_s = ConcatenateStrings (S_CROP_ONTOLOGY_API_URL_S, term_s);

			if (url_s)
				{
					CurlTool *tool_p = AllocateMemoryCurlTool (0);

					if (tool_p)
						{
							if (SetUriForCurlTool (tool_p, url_s))
								{
									CURLcode c = RunCurlTool (tool_p);

									if (c == CURLE_OK)
										{
											const char *results_s = GetCurlToolData (tool_p);

											if (results_s)
												{
													json_error_t err;
													json_t *res_p = json_loads (results_s, JSON_DECODE_ANY, &err);

													if (res_p)
														{
															if (json_is_array (res_p))
																{
																	const size_t num_results = json_array_size (res_p);
																	size_t i = 0;
																	TermType tt = TT_NUM_TYPES;

																	/* Determine the type of data */
																	while ((i < num_results) && (tt == TT_NUM_TYPES))
																		{
																			json_t *entry_p = json_array_get (res_p, i);
																			const char *key_s = GetJSONString (entry_p, "key");

																			if (key_s)
																				{
																					if (strcmp (key_s, "Method name") == 0)
																						{
																							tt = TT_METHOD;
																						}
																					else if (strcmp (key_s, "Variable name") == 0)
																						{
																							tt = TT_VARIABLE;
																						}
																					else if (strcmp (key_s, "Trait name") == 0)
																						{
																							tt = TT_TRAIT;
																						}
																					else if (strcmp (key_s, "Scale name") == 0)
																						{
																							tt = TT_UNIT;
																						}

																				}		/* if (key_s) */

																			++ i;
																		}		/* while ((i < num_results) && (var_type == TT_NUM_TYPES)) */


																	if (tt != TT_NUM_TYPES)
																		{
																			/*
																			 * Does the term type match what we expected to get?
																			 */
																			if ((expected_type == TT_NUM_TYPES) || (tt == expected_type))
																				{
																					const char *name_key_s = NULL;
																					const char *abbr_key_s = NULL;
																					const char *desc_key_s = NULL;
																					bool is_unit_flag = false;

																					switch (tt)
																						{
																							case TT_TRAIT:
																								name_key_s = "Trait name";
																								abbr_key_s = "Main trait abbreviation";
																								desc_key_s = "Trait description";
																								break;

																							case TT_METHOD:
																								name_key_s = "Method name";
																								desc_key_s = "Method description";
																								break;

																							case TT_UNIT:
																								name_key_s = "Scale name";
																								is_unit_flag = true;
																								break;

																							case TT_VARIABLE:
																								name_key_s = "Variable name";
																								break;

																							default:
																								break;
																						}

																					term_p = GetSchemaTerm (res_p, term_s, name_key_s, abbr_key_s, desc_key_s, is_unit_flag);

																					if (!term_p)
																						{
																							PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, res_p, "GetSchemaTerm failed for term \"%s\", name_key \"%s\", abbr_key \"%s\", desc_key \"%s\"", term_s, name_key_s ? name_key_s : "", abbr_key_s ? abbr_key_s : "", desc_key_s ? desc_key_s : "");
																						}
																					else
																						{
																							*found_type_p = tt;
																						}

																				}		/* if (tt == expected_type) */
																			else
																				{
																					PrintJSONToLog (STM_LEVEL_WARNING, __FILE__, __LINE__, res_p, "term is of type %d, expected is %d", tt, expected_type);
																				}



																		}		/* if (tt != TT_NUM_TYPES) */
																	else
																		{
																			PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, res_p, "Failed to determine term type");
																		}

																}		/* if (json_is_array (res_p)) */

															json_decref (res_p);
														}		/* if (res_p) */
													else
														{
															PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "failed to load \"%s\" as JSON, err at %d, %d", results_s, err.line, err.column);
														}

												}		/* if (results_s) */
											else
												{
													PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "failed to get data for \"%s\"", url_s);
												}

										}		/* if (c == CURLE_OK) */
									else
										{
											const char *error_s = curl_easy_strerror (c);

											PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "failed to run \"%s\": \"%s\"", url_s, error_s);
										}

								}		/* if (SetUriForCurlTool (tool_p, url_s)) */
							else
								{
									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "failed to get set curl tool's url to \"%s\"", url_s);
								}

							FreeCurlTool (tool_p);
						}		/* if (tool_p) */
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "failed to allocate curl tool for \"%s\"", url_s);
						}

					FreeCopiedString (url_s);
				}		/* if (url_s) */
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "failed to make url from \"%s\" and \"%s\"", S_CROP_ONTOLOGY_API_URL_S, crop_ontology_term_s);
				}

		}		/* if (!term_p) */


	return term_p;
}


/*
 * static definitions
 */


static char *GetTermEnglishValue (const json_t *entry_p)
{
	char *term_value_s = NULL;
	const char *value_s = GetJSONString (entry_p, "value");

	if (value_s)
		{
			json_error_t err;
			json_t *data_p = json_loads (value_s, 0, &err);

			if (data_p)
				{
					value_s = GetJSONString (data_p, "english");

					if (value_s)
						{
							term_value_s = EasyCopyToNewString (value_s);
						}

					json_decref (data_p);
				}

		}

	return term_value_s;
}


static SchemaTerm *GetSchemaTerm (json_t *values_p, const char *term_s, const char *name_key_s, const char *abbreviation_key_s, const char *description_key_s, const bool is_unit_term_flag)
{
	SchemaTerm *term_p = NULL;
	const size_t num_results = json_array_size (values_p);
	size_t i = 0;
	size_t matched_count = 0;
	char *name_s = NULL;
	char *description_s = NULL;
	char *abbreviation_s = NULL;
	size_t num_to_match = 0;
	char *scale_name_s = NULL;
	char *lower_limit_s = NULL;
	char *upper_limit_s = NULL;
	char *decimal_place_s = NULL;
	char *scale_class_s = NULL;


	if (name_key_s)
		{
			++ num_to_match;
		}

	if (abbreviation_key_s)
		{
			++ num_to_match;
		}

	if (description_key_s)
		{
			++ num_to_match;
		}


	if (is_unit_term_flag)
		{
			num_to_match += 4;
		}

	while ((i < num_results) && (matched_count < num_to_match))
		{
			json_t *entry_p = json_array_get (values_p, i);
			const char *key_s = GetJSONString (entry_p, "key");

			if (key_s)
				{
					if ((name_key_s != NULL) && (strcmp (key_s, name_key_s) == 0))
						{
							name_s = GetTermEnglishValue (entry_p);
							++ matched_count;
						}
					else if ((description_key_s != NULL) && (strcmp (key_s, description_key_s) == 0))
						{
							description_s = GetTermEnglishValue (entry_p);
							++ matched_count;
						}
					else if ((abbreviation_key_s != NULL) && (strcmp (key_s, abbreviation_key_s) == 0))
						{
							abbreviation_s = GetTermEnglishValue (entry_p);
							++ matched_count;
						}
					else if (is_unit_term_flag)
						{
							if (strcmp (key_s, "Scale name") == 0)
								{
									scale_name_s = GetTermEnglishValue (entry_p);
								}
							else if (strcmp (key_s, "Upper limit"))
								{
									upper_limit_s = GetTermEnglishValue (entry_p);
								}
							else if (strcmp (key_s, "lower limit"))
								{
									lower_limit_s = GetTermEnglishValue (entry_p);
								}
							else if (strcmp (key_s, "Decimal places"))
								{
									decimal_place_s = GetTermEnglishValue (entry_p);
								}
							else if (strcmp (key_s, "Scale class"))
								{
									scale_class_s = GetTermEnglishValue (entry_p);
								}
						}
				}

			++ i;
		}		/* while ((i < num_results) && (matched_count < num_to_match)) */


	if (num_to_match == matched_count)
		{
			term_p = AllocateExtendedSchemaTerm (term_s, name_s, description_s, abbreviation_s);

			if (!term_p)
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "failed to allocate term for \"%s\", \"%s\", \"%s\", \"%s\"", term_s, name_s, description_s, abbreviation_s);
				}

			if (name_s)
				{
					FreeCopiedString (name_s);
				}

			if (description_s)
				{
					FreeCopiedString (description_s);
				}

			if (abbreviation_s)
				{
					FreeCopiedString (abbreviation_s);
				}

		}

	return term_p;
}


static SchemaTerm *FindCachedCropOnotologySchemaTerm (const char *term_s, const char *key_stem_s, MongoTool *tool_p, bson_t *opts_p)
{
	SchemaTerm *term_p = NULL;
	json_t *query_p = json_object ();

	if (query_p)
		{
			char *key_s = ConcatenateVarargsStrings (key_stem_s, ".", SCHEMA_TERM_URL_S, NULL);

			if (key_s)
				{
					if (SetJSONString (query_p, key_s, term_s))
						{
							if (FindMatchingMongoDocumentsByJSON (tool_p, query_p, NULL, opts_p))
								{
									json_t *results_p = GetAllExistingMongoResultsAsJSON (tool_p);

									if (results_p)
										{
											if (json_is_array (results_p))
												{
													if (json_array_size (results_p) > 0)
														{
															json_t *result_p = json_array_get (results_p, 0);
															json_t *term_json_p = json_object_get (result_p, key_stem_s);

															if (term_json_p)
																{
																	term_p = GetSchemaTermFromJSON (term_json_p);

																	if (!term_p)
																		{
																			PrintJSONToErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, term_json_p, "GetSchemaTermFromJSON failed for \"%s\": \"%s\" query", key_s, term_s);
																		}
																}		/* if (term_json_p) */
															else
																{
																	PrintJSONToErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, result_p, "Failed to get \"%s\": from result", key_stem_s);
																}

														}		/* if (json_array_size (results_p) > 0) */

												}		/* 	if (json_is_array (results_p)) */
											else
												{
													PrintJSONToErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, results_p, "results_p is not an array for \"%s\": \"%s\" query", key_s, term_s);
												}

										}		/* if (results_p) */
									else
										{

											PrintJSONToErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, query_p, "GetAllExistingMongoResultsAsJSON failed for query");
										}

								}		/* if (FindMatchingMongoDocumentsByJSON (tool_p, query_p, NULL, opts_p)) */
							else
								{
									PrintJSONToErrors (STM_LEVEL_FINER, __FILE__, __LINE__, query_p, "FindMatchingMongoDocumentsByJSON failed");
								}

						}		/* if (SetJSONString (query_p, key_s, term_s)) */
					else
						{
							PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Failed to set \"%s\": \"%s\" in query JSON", key_s, term_s);
						}

					FreeCopiedString (key_s);
				}

			json_decref (query_p);
		}

	return term_p;
}



static SchemaTerm *GetCachedCropOnotologySchemaTerm (const char *term_s, MongoTool *tool_p)
{
	SchemaTerm *term_p = NULL;

	/* Any hit will do, so limit our results size to 1 */
	bson_t *opts_p = BCON_NEW ("limit", BCON_INT64 (1));

	if (opts_p)
		{
			if ((term_p = FindCachedCropOnotologySchemaTerm (term_s, MV_TRAIT_S, tool_p, opts_p)) == NULL)
				{
					if ((term_p = FindCachedCropOnotologySchemaTerm (term_s, MV_MEASUREMENT_S, tool_p, opts_p)) == NULL)
						{
							if ((term_p = FindCachedCropOnotologySchemaTerm (term_s, MV_UNIT_S, tool_p, opts_p)) == NULL)
								{
									if ((term_p = FindCachedCropOnotologySchemaTerm (term_s, MV_VARIABLE_S, tool_p, opts_p)) == NULL)
										{

										}

								}

						}

				}

			bson_destroy (opts_p);
		}		/* if (opts_p) */

	return term_p;
}


/*
[
  {
    "key": "name",
    "value": "{\"english\":\"cm\"}"
  },
  {
    "key": "created_at",
    "value": "Mon May 30 11:21:30 UTC 2016"
  },
  {
    "key": "ontology_id",
    "value": "CO_327"
  },
  {
    "key": "ontology_name",
    "value": "Pearl millet"
  },
  {
    "key": "Lower limit",
    "value": "{\"english\":\"0.0\"}"
  },
  {
    "key": "Scale name",
    "value": "{\"english\":\"cm\"}"
  },
  {
    "key": "language",
    "value": "EN"
  },
  {
    "key": "Scale class",
    "value": "{\"english\":\"Numerical\"}"
  }
]


[
  {
    "key": "name",
    "value": "{\"english\":\"Visual Flower color scale\"}"
  },
  {
    "key": "created_at",
    "value": "Tue Sep 03 07:52:43 UTC 2019"
  },
  {
    "key": "ontology_id",
    "value": "CO_341"
  },
  {
    "key": "ontology_name",
    "value": "Pigeonpea"
  },
  {
    "key": "Scale name",
    "value": "{\"english\":\"Visual Flower color scale\"}"
  },
  {
    "key": "Category 3",
    "value": "{\"english\":\"3= orange yellow\"}"
  },
  {
    "key": "Category 2",
    "value": "{\"english\":\"2= yellow\"}"
  },
  {
    "key": "Category 5",
    "value": "{\"english\":\"5= red\"}"
  },
  {
    "key": "language",
    "value": "EN"
  },
  {
    "key": "Category 4",
    "value": "{\"english\":\"4= purple\"}"
  },
  {
    "key": "Scale class",
    "value": "{\"english\":\"Nominal\"}"
  },
  {
    "key": "Category 1",
    "value": "{\"english\":\"1= Light yellow\"}"
  }
]

*/

json_t *GetScaleClassAsEmbeddedJSON (const ScaleClass *class_p, const char *key_s)
{
	json_t *obj_p = json_object ();

	if (obj_p)
		{
			json_t *sc_json_p = GetScaleClassAsJSON (class_p);

			if (sc_json_p)
				{
					if (json_object_set_new (obj_p, key_s, sc_json_p) == 0)
						{
							return obj_p;
						}
					else
						{
							json_decref (sc_json_p);
						}
				}

			json_decref (obj_p);
		}

	return NULL;
}


json_t *GetScaleClassAsJSON (const ScaleClass *class_p)
{
	json_t *sc_json_p = json_object ();

	if (sc_json_p)
		{
			if (SetJSONString (sc_json_p, S_SCALE_CLASS_NAME_S, class_p -> sc_name_s))
				{
					const char *type_s = GetGrassrootsTypeAsString (class_p -> sc_type);

					if (SetJSONString (sc_json_p, S_SCALE_CLASS_TYPE_S, type_s))
						{
							return sc_json_p;
						}
				}

			json_decref (sc_json_p);
		}

	return NULL;
}


const ScaleClass *GetScaleClassFromJSON (const json_t *class_json_p)
{
	const ScaleClass *class_p = NULL;
	const char *name_s = GetJSONString (class_json_p, S_SCALE_CLASS_NAME_S);

	if (name_s)
		{
			const char *type_s = GetJSONString (class_json_p, S_SCALE_CLASS_TYPE_S);

			if (type_s)
				{
					ParameterType pt;

					if (GetGrassrootsTypeFromString (type_s, &pt))
						{
							class_p = GetScaleClassByName (name_s);

							if (class_p)
								{
									if (class_p -> sc_type == pt)
										{
											return class_p;
										}
									else
										{
											PrintJSONToErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, class_json_p, "class %s has sc_type %d that does not match %d", class_p -> sc_name_s, class_p -> sc_type, pt);
										}
								}
							else
								{
									PrintJSONToErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, class_json_p, "GetScaleClassByName () failed for \"%s\"", name_s);
								}
						}
					else
						{
							PrintJSONToErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, class_json_p, "GetGrassrootsTypeFromString () for \"%s\"", type_s);
						}
				}
			else
				{
					PrintJSONToErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, class_json_p, "Failed to find \"%s\"", S_SCALE_CLASS_TYPE_S);
				}
		}
	else
		{
			PrintJSONToErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, class_json_p, "Failed to find \"%s\"", S_SCALE_CLASS_NAME_S);
		}

	return NULL;
}


OperationStatus StoreAllScaleUnits (FieldTrialServiceData *data_p)
{
	OperationStatus status = OS_FAILED;
	CurlTool *curl_tool_p = AllocateMemoryCurlTool (0);

	if (curl_tool_p)
		{
			MongoTool *mongo_p = GetConfiguredMongoTool (data_p, NULL);

			if (mongo_p)
				{
					json_t *query_p = json_object ();

					if (query_p)
						{
							COToolData tool_data;

							tool_data.cotd_service_data_p = data_p;
							tool_data.cotd_query_p = query_p;
							tool_data.cotd_curl_p = curl_tool_p;
							tool_data.cotd_query_key_s = "variable.so:sameAs";

							if (SetMongoToolCollection (mongo_p, data_p -> dftsd_collection_ss [DFTD_MEASURED_VARIABLE]))
								{
									status = ProcessMongoResults (mongo_p, NULL, NULL, UpdateScaleTerms, &tool_data);
								}

							json_decref (query_p);
						}

					FreeMongoTool (mongo_p);
				}		/* if (mongo_p) */
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "GetConfiguredMongoTool () failed");
				}


			FreeCurlTool (curl_tool_p);
		}

	return status;
}


static bool UpdateScaleTerms (const bson_t *document_p, void *data_p)
{
	bool success_flag = false;
	json_t *mv_json_p = ConvertBSONToJSON (document_p, NULL);

	if (mv_json_p)
		{
			COToolData *tool_data_p = (COToolData *) data_p;
			MeasuredVariable *var_p = GetMeasuredVariableFromJSON (mv_json_p, tool_data_p -> cotd_service_data_p);

			if (var_p)
				{
					const char *var_url_s = GetMeasuredVariableURL (var_p);

					if (var_url_s)
						{
							/* All crop ontology variables begin with CO: */
							if (DoesStringStartWith (var_url_s, "CO_"))
								{
									const ScaleClass *scale_class_p = GetScaleClass (var_url_s, tool_data_p -> cotd_curl_p);

									if (scale_class_p)
										{
											/* update the variable in the db */
											json_t *scale_class_json_p = GetScaleClassAsEmbeddedJSON (scale_class_p, MV_SCALE_S);

											if (scale_class_json_p)
												{
													if (SetJSONString (tool_data_p -> cotd_query_p, tool_data_p -> cotd_query_key_s, var_url_s))
														{
															MongoTool *mongo_p = GetConfiguredMongoTool (tool_data_p -> cotd_service_data_p, NULL);

															if (mongo_p)
																{
																	if (UpdateMongoDocumentsByJSON (mongo_p, tool_data_p -> cotd_query_p, scale_class_json_p, false))
																		{
																			success_flag = true;
																		}

																	FreeMongoTool (mongo_p);
																}
															else
																{
																	PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "GetConfiguredMongoTool () failed");
																}

														}

													json_decref (scale_class_json_p);
												}
										}
								}

						}

					FreeMeasuredVariable (var_p);
				}

			json_decref (mv_json_p);
		}

	return success_flag;
}






/*
 * Iterate over each variable in db
 *
 * get https://cropontology.org/brapi/v1/variables/CO_321:0001199
 *
 * get result.scale.dataType
 *
 *
 */


static const ScaleClass *GetScaleClass (const char *variable_url_s, CurlTool *curl_p)
{
	const ScaleClass *class_p = NULL;
	const char * const CO_API_URL_S = "https://cropontology.org/brapi/v1/variables/";
	char *url_s = ConcatenateStrings (CO_API_URL_S, variable_url_s);

	if (url_s)
		{
			if (SetUriForCurlTool (curl_p, url_s))
				{
					CURLcode res;

					ClearCurlToolData (curl_p);

					res = RunCurlTool (curl_p);

					if (res == CURLE_OK)
						{
							const char *data_s = GetCurlToolData (curl_p);

							if (data_s)
								{
									json_error_t err;
									json_t *res_p = json_loads (data_s, 0, &err);

									if (res_p)
										{
											class_p = GetScaleDatatype (res_p);

											if (!class_p)
												{

												}		/* if (!class_p) */

											json_decref (res_p);
										}		/* if (res_p) */
									else
										{

										}

								}		/* if (data_s) */
							else
								{

								}

						}		/* if (res == CURLE_OK) */
					else
						{

						}
				}		/* if (SetUriForCurlTool (curl_p, url_s)) */
			else
				{

				}

			FreeCopiedString (url_s);
		}		/* if (url_s) */
	else
		{

		}

	return class_p;
}


static const ScaleClass *GetScaleDatatype (const json_t *document_p)
{
	json_t *result_p = json_object_get (document_p, "result");

	if (result_p)
		{
			json_t *scale_p = json_object_get (result_p, "scale");

			if (scale_p)
				{
					const char *dt_s = GetJSONString (scale_p, "dataType");

					if (dt_s)
						{
							return GetScaleClassByName (dt_s);
						}

				}
		}

	return NULL;
}



const ScaleClass *GetScaleClassByName (const char * const name_s)
{
	const ScaleClass *SCALES_PP [] =
	{
		&SCALE_DURATION,
		&SCALE_NOMINAL,
		&SCALE_NUMERICAL,
		&SCALE_CODE,
		&SCALE_ORDINAL,
		&SCALE_TEXT,
		&SCALE_DATE,
		NULL
	};

	const ScaleClass **class_pp = SCALES_PP;

	while (*class_pp)
		{
			if (strcmp (name_s, (*class_pp) -> sc_name_s) == 0)
				{
					return *class_pp;
				}
			else
				{
					++ class_pp;
				}
		}

	return NULL;
}

