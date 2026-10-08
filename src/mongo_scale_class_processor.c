/*
 * mongo_scale_class_processor.c
 *
 *  Created on: 31 Mar 2022
 *      Author: billy
 */

#include <stdio.h>

#include "bson/bson.h"
#include "mongoc/mongoc.h"

#include "streams.h"

#include "mongodb_tool.h"

#include "parameter_type.h"
#include "json_util.h"
#include "time_util.h"

#define ALLOCATE_OBSERVATION_TAGS (1)
#include "observation.h"


typedef enum
{
	VS_FAIL,
	VS_REMOVE,
	VS_NEEDS_UPDATE,
	VS_OK,
} ValueStatus;


/*
 * static declarations
 */

static json_t *GetNextDocAsJSON (mongoc_cursor_t *cursor_p);

static ValueStatus SetInteger (json_t *observation_json_p, const char * const key_s);

static ValueStatus SetReal (json_t *observation_json_p, const char * const key_s);

static ValueStatus CheckString (json_t *observation_json_p, const char * const key_s);

static ValueStatus CheckTime (json_t *observation_json_p, const char * const key_s);

static bool GetPhenotypeDatatype (const json_t *phenotype_json_p, ParameterType *param_type_p);

static bool WritePlotRows (bson_oid_t *plot_id_p, mongoc_collection_t *plots_collection_p, json_t *rows_p);

static bool IsEmptyEntry (const char *value_s);

static const char *LocalGetObservationTypeAsString (const ObservationType obs_type);

static ValueStatus UpdateType (json_t * const observation_json_p, const ObservationType obs_type);



int main (int argc, char *argv [])
{
	mongoc_client_t *client_p;
	const char *db_s = "dfw_field_trial";
	const char *plots_collection_s = "Plots";
	const char *phenotypes_collection_s = "Phenotypes";
	int arg_index = 1;

	while (arg_index < argc)
		{
			if (strcmp (argv [arg_index], "--db") == 0)
				{
					if ((arg_index + 1) < argc)
						{
							db_s = argv [++ arg_index];
						}
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Database argument missing\n");
						}
				}
			else if (strcmp (argv [arg_index], "--plots") == 0)
				{
					if ((arg_index + 1) < argc)
						{
							plots_collection_s = argv [++ arg_index];
						}
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Plots argument missing\n");
						}
				}
			else if (strcmp (argv [arg_index], "--phenotypes") == 0)
				{
					if ((arg_index + 1) < argc)
						{
							phenotypes_collection_s = argv [++ arg_index];
						}
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Phenotypes argument missing\n");
						}
				}			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Unknown argument: \"%s\"\n", argv [arg_index]);
				}

			++ arg_index;
		}		/* while (arg_index < argc) */


	mongoc_init ();

	client_p = mongoc_client_new ("mongodb://localhost:27017/?appname=set_datatypes");

	if (client_p)
		{
			mongoc_collection_t *plots_collection_p = mongoc_client_get_collection (client_p, db_s, plots_collection_s);

			if (plots_collection_p)
				{
					mongoc_collection_t *phenotypes_collection_p = mongoc_client_get_collection (client_p, db_s, phenotypes_collection_s);

					if (phenotypes_collection_p)
						{
							bson_t *query_p;

							/*
							 * Iterate over each plot and get all of the rows and
							 * subsequently their observations.
							 *
							 * For each observation, get its raw and corrected values
							 * and check if they can be converted into the correct
							 * datatype as specified by the phenotype's scale class
							 */

							query_p = bson_new ();

							if (query_p)
								{
									mongoc_cursor_t *plots_cursor_p = mongoc_collection_find_with_opts (plots_collection_p, query_p, NULL, NULL);

									if (plots_cursor_p)
										{
											size_t num_succeeded = 0;
											size_t num_failed = 0;
											size_t num_ignored = 0;
											size_t num_removed = 0;

											json_t *plot_json_p = NULL;

											while ((plot_json_p = GetNextDocAsJSON (plots_cursor_p)) != NULL)
												{
													json_t *rows_p = json_object_get (plot_json_p, "rows");

													if (rows_p)
														{
															if (json_is_array (rows_p))
																{
																	size_t i;
																	json_t *row_p;
																	bool plot_updated_flag = false;

																	json_array_foreach (rows_p, i, row_p)
																		{
																			json_t *observations_p = json_object_get (row_p, "observations");

																			if (observations_p)
																				{
																					if (json_is_array (observations_p))
																						{
																							size_t j = 0;
																							bson_t phenotype_query;
																							bool first_flag = true;
																							bool error_flag = false;
																							size_t num_observations = json_array_size (observations_p);

																							while (j < num_observations)
																								{
																									bson_oid_t phenotype_id;
																									json_t *observation_p = json_array_get (observations_p, j);
																									bool remove_flag = false;


																									if (GetNamedIdFromJSON (observation_p, "phenotype_id", &phenotype_id))
																										{
																											if (first_flag)
																												{
																													bson_init (&phenotype_query);
																													first_flag = false;
																												}
																											else
																												{
																													bson_reinit (&phenotype_query);
																												}

																											if (BSON_APPEND_OID (&phenotype_query, MONGO_ID_S, &phenotype_id))
																												{
																													mongoc_cursor_t *phenotypes_cursor_p = mongoc_collection_find_with_opts (phenotypes_collection_p, &phenotype_query, NULL, NULL);

																													if (phenotypes_cursor_p)
																														{
																															json_t *phenotype_json_p = GetNextDocAsJSON (phenotypes_cursor_p);

																															if (phenotype_json_p)
																																{
																																	ParameterType pt;
																																	const char * const RAW_KEY_S = "raw_value";
																																	const char * const CORRECTED_KEY_S = "corrected_value";

																																	if (GetPhenotypeDatatype (phenotype_json_p, &pt))
																																		{
																																			ValueStatus raw_ret = VS_OK;
																																			ValueStatus corrected_ret = VS_OK;

																																			switch (pt)
																																				{
																																					case PT_SIGNED_INT:
																																						{
																																							raw_ret = SetInteger (observation_p, RAW_KEY_S);
																																							corrected_ret = SetInteger (observation_p, CORRECTED_KEY_S);
																																						}
																																						break;

																																					case PT_SIGNED_REAL:
																																						{
																																							raw_ret = SetReal (observation_p, RAW_KEY_S);
																																							corrected_ret = SetReal (observation_p, CORRECTED_KEY_S);
																																						}
																																						break;

																																					case PT_TIME:
																																						{
																																							raw_ret = CheckTime (observation_p, RAW_KEY_S);
																																							corrected_ret = CheckTime (observation_p, CORRECTED_KEY_S);
																																						}
																																						break;

																																					case PT_STRING:
																																						{
																																							raw_ret = CheckString (observation_p, RAW_KEY_S);
																																							corrected_ret = CheckString (observation_p, CORRECTED_KEY_S);
																																						}
																																						break;

																																					default:
																																						break;

																																				}		/* switch (pt) */

																																			if ((raw_ret == VS_FAIL) || (corrected_ret == VS_FAIL))
																																				{
																																					/* An error occurred */
																																					error_flag = true;
																																					PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, plot_json_p, "Error occurred for plot");

																																				}
																																			else if ((raw_ret == VS_NEEDS_UPDATE) || (corrected_ret == VS_NEEDS_UPDATE))
																																				{
																																					/*
																																					 * The observation's values have been updated so we'll need to
																																					 * save it back to the database
																																					 */
																																					plot_updated_flag = true;
																																				}
																																			else if ((raw_ret == VS_REMOVE) || (corrected_ret == VS_REMOVE))
																																				{
																																					remove_flag = true;
																																				}
																																			else if ((raw_ret == VS_OK) || (corrected_ret == VS_OK))
																																				{
																																				}

																																		}		/* if (GetPhenotypeDatatype (phenotype_json_p, &pt)) */
																																	else
																																		{
																																			PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, phenotype_json_p, "GetPhenotypeDatatype () failed");
																																			error_flag = true;
																																		}

																																	json_decref (phenotype_json_p);
																																}		/* if (phenotype_json_p) */

																															mongoc_cursor_destroy (phenotypes_cursor_p);
																														}		/* if (phenotypes_cursor_p) */
																													else
																														{
																															char *phenotype_id_s = GetBSONOidAsString (&phenotype_id);

																															if (phenotype_id_s)
																																{
																																	PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to find phenotype with id \"%s\"", phenotype_id_s);
																																	FreeBSONOidString (phenotype_id_s);
																																}
																															else
																																{
																																	PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to find phenotype");
																																}

																															error_flag = true;
																														}

																												}		/* if (BSON_APPEND_OID (&phenotype_query, MONGO_ID_S, &phenotype_id)) */
																											else
																												{
																													char *phenotype_id_s = GetBSONOidAsString (&phenotype_id);

																													if (phenotype_id_s)
																														{
																															PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to append \"%s\": \"%s\" to phenotype query", MONGO_ID_S, phenotype_id_s);
																															FreeBSONOidString (phenotype_id_s);
																														}
																													else
																														{
																															PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to append id to phenotype query");
																														}

																													error_flag = true;
																												}

																										}		/* if (GetNamedIdFromJSON (observation_p, "phenotype_id", &phenotype_id)) */
																									else
																										{
																											PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, observation_p, "Failed to get phenotype_id");
																										}

																									if (remove_flag)
																										{
																											/* Remove the entry */
																											if (json_array_remove (observations_p, j) == 0)
																												{
																													plot_updated_flag = true;
																													-- num_observations;
																													++ num_removed;
																													PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, observations_p, "Removed entry " SIZET_FMT, j);
																												}
																											else
																												{
																													error_flag = true;
																													PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, observations_p, "Error removing entry " SIZET_FMT, j);
																													++ j;
																												}

																										}
																									else
																										{
																											++ j;
																										}


																								}		/* while (j < num_observations) */


																							if (error_flag)
																								{
																									++ num_failed;
																								}

																						}		/* if (json_is_array (observations_p)) */

																				}		/* if (observations_p) */

																		}		/* json_array_foreach (rows_p, i, row_p) */

																	/*
																	 * Do we need to update the database?
																	 */
																	if (plot_updated_flag)
																		{
																			bson_oid_t plot_id;

																			if (GetMongoIdFromJSON (plot_json_p, &plot_id))
																				{
																					char *id_s = GetBSONOidAsString (&plot_id);

																					if (id_s)
																						{
																							PrintLog (STM_LEVEL_INFO, __FILE__, __LINE__, "About to update plot \"%s\"\n", id_s);
																							FreeBSONOidString (id_s);
																						}
																					else
																						{
																							PrintLog (STM_LEVEL_INFO, __FILE__, __LINE__, "About to update unknown plot\n");
																						}

																					if (WritePlotRows (&plot_id, plots_collection_p, rows_p))
																						{
																							++ num_succeeded;
																						}
																					else
																						{
																							PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, plot_json_p, "WritePlot () failed");
																						}
																				}
																			else
																				{
																					PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, plot_json_p, "Failed to get plot_id");
																				}

																		}		/* if (plot_updated_flag) */
																	else
																		{
																			++ num_ignored;
																		}

																}		/* if (json_is_array (rows_p)) */
														}

													json_decref (plot_json_p);
												}		/* while ((plot_json_p = GetNextDocAsJSON (plots_cursor_p)) != NULL) */


											PrintLog (STM_LEVEL_INFO, __FILE__, __LINE__, "num plots updated = " SIZET_FMT " num observations removed = " SIZET_FMT " num plots ignored = " SIZET_FMT "\n",  num_succeeded, num_removed, num_ignored);

											mongoc_cursor_destroy (plots_cursor_p);
										}		/* if (plots_cursor_p) */
									else
										{
											PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to get plots cursor");
										}

									bson_destroy (query_p);
								}		/* if (query_p) */
							else
								{
									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to create query");
								}

							mongoc_collection_destroy (phenotypes_collection_p);
						}		/* if (phenotypes_collection_p) */
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to get phenotypes collection");
						}

					mongoc_collection_destroy (plots_collection_p);
				}		/* if (plots_collection_p) */
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to get plots collection");
				}

			mongoc_client_destroy (client_p);
		}		/* if (client_p) */
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to get mongo client");
		}

	mongoc_cleanup ();

	return 0;
}



static json_t *GetNextDocAsJSON (mongoc_cursor_t *cursor_p)
{
	json_t *json_p = NULL;
	const bson_t *doc_p;

	if (mongoc_cursor_next (cursor_p, &doc_p))
		{
			json_p = ConvertBSONToJSON (doc_p, NULL);

			if (!json_p)
				{
					puts ("Failed to convert bson to json");
				}
		}


	return json_p;
}



static ValueStatus SetInteger (json_t *observation_json_p, const char * const key_s)
{
	ValueStatus ret = VS_FAIL;
	const char *value_s = GetJSONString (observation_json_p, key_s);
	int i;

	if (value_s)
		{
			/* If the value is set to empty or "na", remove it */
			if ((IsEmptyEntry (value_s)) || (Stricmp (value_s, "na") == 0))
				{
					ret = VS_REMOVE;
				}
			else
				{
					int res = sscanf (value_s, "%d", &i);

					if (res == 1)
						{
							if (SetJSONInteger (observation_json_p, key_s, i))
								{
									ret = VS_NEEDS_UPDATE;
								}
							else
								{
									PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, observation_json_p, "Failed to set integer \"%s\": %d", key_s, i);
								}
						}
					else
						{
							PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, observation_json_p, "Failed to convert \"%s\" to an integer, err: %d", value_s, res);
						}
				}
		}
	else
		{
			/* no existing value set */
			ret = VS_OK;
		}


	if (ret != VS_FAIL)
		{
			ValueStatus type_status = UpdateType (observation_json_p, OT_INTEGER);

			if (type_status != VS_OK)
				{
					ret = type_status;
				}
		}


	return ret;
}


static ValueStatus CheckString (json_t *observation_json_p, const char * const key_s)
{
	ValueStatus ret = VS_FAIL;
	json_t *value_p = json_object_get (observation_json_p, key_s);


	if (value_p)
		{
			if ((json_is_string (value_p)) || (json_is_null (value_p)))
				{
					ret = VS_OK;
				}
			else
				{
					PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, observation_json_p, "Stored value for \"%s\" is not a string: %d", key_s, json_typeof (value_p));
				}
		}
	else
		{
			/* no existing value set */
			ret = VS_OK;
		}


	if (ret != VS_FAIL)
		{
			ValueStatus type_status = UpdateType (observation_json_p, OT_STRING);

			if (type_status != VS_OK)
				{
					ret = type_status;
				}
		}

	return ret;
}


static ValueStatus CheckTime (json_t *observation_json_p, const char * const key_s)
{
	ValueStatus ret = VS_FAIL;
	const char *time_s = GetJSONString (observation_json_p, key_s);

	if (time_s)
		{
			bool time_set_flag = false;
			struct tm time_val;

			if (SetTimeFromString (&time_val, time_s))
				{
					time_set_flag = true;
				}
			else if (SetTimeFromDDMMYYYYString (&time_val, time_s))
				{
					time_set_flag = true;
				}
			else
				{
					PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, observation_json_p, "Stored value for \"%s\", \"%s\" is not a time", key_s, time_s);
				}


			if (time_set_flag)
				{
					bool include_time_flag = false;
					char *parsed_time_s = NULL;

					if ((time_val.tm_hour != 0) || (time_val.tm_min != 0) || (time_val.tm_sec != 0))
						{
							include_time_flag = true;
						}

					parsed_time_s = GetTimeAsString (&time_val, include_time_flag, NULL);


					if (parsed_time_s)
						{
							bool times_match_flag = (strcmp (time_s, parsed_time_s) == 0);

							if (times_match_flag)
								{
									ret = VS_OK;
								}
							else
								{
									if (SetJSONString (observation_json_p, key_s, parsed_time_s))
										{
											ret = VS_NEEDS_UPDATE;
										}
									else
										{
											PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, observation_json_p, "Failed to set time \"%s\": \"%s\"", key_s, parsed_time_s);
										}
								}

							FreeCopiedString (parsed_time_s);
						}		/* if (parsed_time_s) */
					else
						{
							PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, observation_json_p, "GetTimeAsString () failed for \"%s\"", key_s);
						}

				}		/* if (time_set_flag) */

		}		/* if (time_s) */
	else
		{
			/* no existing value set */
			ret = VS_OK;
		}


	if (ret != VS_FAIL)
		{
			ValueStatus type_status = UpdateType (observation_json_p, OT_TIME);

			if (type_status != VS_OK)
				{
					ret = type_status;
				}
		}


	return ret;
}



static ValueStatus SetReal (json_t *observation_json_p, const char * const key_s)
{
	ValueStatus ret = VS_FAIL;
	const char *value_s = GetJSONString (observation_json_p, key_s);
	double d;

	if (value_s)
		{
			/* If the value is set to empty or "na", remove it */
			if ((IsEmptyEntry (value_s)) || (strcmp (value_s, "na") == 0))
				{
					ret = VS_REMOVE;
				}
			else
				{
					int res = sscanf (value_s, "%lf", &d);

					if (res == 1)
						{
							if (SetJSONReal (observation_json_p, key_s, d))
								{
									ret = VS_NEEDS_UPDATE;
								}
							else
								{
									PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, observation_json_p, "Failed to set double \"%s\": %lf", key_s, d);
								}
						}
					else
						{
							PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, observation_json_p, "Failed to convert \"%s\" to a double, err: %d", value_s, res);
						}
				}
		}
	else
		{
			/* no existing value set */
			ret = VS_OK;
		}

	if (ret != VS_FAIL)
		{
			ValueStatus type_status = UpdateType (observation_json_p, OT_NUMERIC);

			if (type_status != VS_OK)
				{
					ret = type_status;
				}
		}

	return ret;
}


/**
 *
 * @return VS_FAIL for failure, VS_OK for no change and VS_NEEDS_UPDATE if the json has been updated
 */
static ValueStatus UpdateType (json_t * const observation_json_p, const ObservationType obs_type)
{
	int ret = VS_FAIL;
	const char *type_s = LocalGetObservationTypeAsString (obs_type);

	if (type_s)
		{
			const char *existing_type_s = GetJSONString (observation_json_p, OB_TYPE_S);

			if ((!existing_type_s) || (strcmp (type_s, existing_type_s) != 0))
				{
					if (SetJSONString (observation_json_p, OB_TYPE_S, type_s))
						{
							ret = VS_NEEDS_UPDATE;
						}
					else
						{
							PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, observation_json_p, "Failed to set \"%s\": \"%s\"", OB_TYPE_S, type_s);
						}
				}
			else
				{
					ret = VS_OK;
				}
		}
	else
		{
			PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, observation_json_p, "LocalGetObservationTypeAsString (%d) returned NULL", obs_type);
		}

	return ret;
}


static bool GetPhenotypeDatatype (const json_t *phenotype_json_p, ParameterType *param_type_p)
{
	bool success_flag = false;
	const json_t *scale_json_p = json_object_get (phenotype_json_p, "scale");

	if (scale_json_p)
		{
			const char *class_s = GetJSONString (scale_json_p, "so:name");

			if (class_s)
				{
					if (strcmp (class_s, "Date") == 0)
						{
							*param_type_p = PT_TIME;
							success_flag = true;
						}
					else if (strcmp (class_s, "Duration") == 0)
						{
							*param_type_p = PT_SIGNED_REAL;
							success_flag = true;
						}
					else if (strcmp (class_s, "Nominal") == 0)
						{
							*param_type_p = PT_STRING;
							success_flag = true;
						}
					else if (strcmp (class_s, "Numerical") == 0)
						{
							*param_type_p = PT_SIGNED_REAL;
							success_flag = true;
						}
					else if (strcmp (class_s, "Ordinal") == 0)
						{
							*param_type_p = PT_SIGNED_INT;
							success_flag = true;
						}
					else if (strcmp (class_s, "Text") == 0)
						{
							*param_type_p = PT_STRING;
							success_flag = true;
						}
					else if (strcmp (class_s, "Code") == 0)
						{
							*param_type_p = PT_STRING;
							success_flag = true;
						}
					else
						{
							PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, phenotype_json_p, "Unknown scale class \"%s\"", class_s);
						}
				}
		}

	return success_flag;
}


static bool WritePlotRows (bson_oid_t *plot_id_p, mongoc_collection_t *plots_collection_p, json_t *rows_p)
{
	bool success_flag = false;
  char *rows_s = json_dumps (rows_p, 0);

  if (rows_s)
  	{
  	  bson_t *query_p = BCON_NEW ("_id", BCON_OID (plot_id_p));

  	  if (query_p)
  	  	{
  	  		bson_t *rows_bson_p = NULL;
  				bson_error_t error;

  				rows_bson_p = bson_new_from_json ((const uint8 *) rows_s, -1, &error);

  				if (rows_bson_p)
  					{
  	  	  		bson_t *update_p = BCON_NEW ("$set",
  	  												 "{",
															 "rows",
															 BCON_ARRAY (rows_bson_p),
  	  												 "}");

  	  				if (update_p)
  	  					{
  	  						bson_t reply;

									if (mongoc_collection_update_one (plots_collection_p, query_p, update_p, NULL, &reply, &error))
										{
											json_t *reply_p = ConvertBSONToJSON (&reply, NULL);

											if (reply_p)
												{
													json_t *modified_p = json_object_get (reply_p, "modifiedCount");

													if (modified_p)
														{
															if (json_is_integer (modified_p))
																{
																	int count = json_integer_value (modified_p);

																	if (count == 1)
																		{
																			success_flag = true;
																		}
																	else
																		{
																			PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, reply_p, "modified value is %d not 1", count);
																		}
																}
															else
																{
																	PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, reply_p, "modified value is not an integer");
																}
														}
													else
														{
															PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, reply_p, "Failed to get modified value");
														}

													json_decref (reply_p);
												}

										}		/* if (mongoc_collection_update_one (plots_collection_p, query_p, update_p, NULL, &reply, &error)) */
									else
										{
		  	  	  	  		PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, rows_p, "mongoc_collection_update_one () failed for \"%s\" with error code: " UINT32_FMT " domain: " UINT32_FMT " message: \"%s\"", rows_s, error.code, error.domain, error.message);
										}

  	  						bson_destroy (update_p);
  	  					}
  	  	  	  else
  	  	  	  	{
  	  	  	  		PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, rows_p, "Failed to create update statement for \"%s\"", rows_s);
  	  	  	  	}

  						bson_destroy (rows_bson_p);
  					}
  	  	  else
  	  	  	{
  	  	  		PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, rows_p, "Failed to create bson from \"%s\"", rows_s);
  	  	  	}

  	  		bson_destroy (query_p);
  	  	}
  	  else
  	  	{
  	  		PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, rows_p, "Failed to create query");
  	  	}

  		free (rows_s);
  	}
  else
  	{
  		PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to get plot json as string");
  	}

  return success_flag;
}


/*
 * Some values have been entered to stand for empty numeric entries
 * which we'll remove to meet the datatype standards
 */
static bool IsEmptyEntry (const char *value_s)
{
	const char *values_ss [] = { "NA", "$$$", NULL };
	const char **value_ss = values_ss;

	while (*value_ss)
		{
			if (strcmp (value_s, *value_ss) == 0)
				{
					return true;
				}
			else
				{
					++ value_ss;
				}
		}

	return false;
}


static const char *S_OBSERVATION_TYPES_SS [OT_NUM_TYPES] = { "xsd:double", "xsd:string", 	"params:signed_integer", "xsd:date"};


static const char *LocalGetObservationTypeAsString (const ObservationType obs_type)
{
	const char *obs_type_s = NULL;

	if (obs_type < OT_NUM_TYPES)
		{
			obs_type_s = * (S_OBSERVATION_TYPES_SS + obs_type);
		}

	return obs_type_s;
}
