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
 * location.c
 *
 *  Created on: 4 Oct 2018
 *      Author: billy
 */

#define ALLOCATE_LOCATION_TAGS (1)
#include "location.h"
#include "memory_allocations.h"
#include "study.h"
#include "dfw_util.h"
#include "indexing.h"
#include "mongodb_util.h"



static void *GetLocationFromJSONCallback (const json_t *location_json_p, const ViewFormat format, const FieldTrialServiceData *data_p);

static bool AddLocationResultToList (const json_t *location_json_p, LinkedList *locations_p, const FieldTrialServiceData *service_data_p);






Location *AllocateLocation (Address *address_p, const uint32 order, const char *soil_s, const double64 *min_ph_p, const double64 *max_ph_p, const LocationType loc_type, bson_oid_t *id_p)
{
	char *copied_soil_s = NULL;

	if (CloneValidString (soil_s, &copied_soil_s))
		{
			double64 *copied_min_ph_p = NULL;

			if (CopyValidReal (min_ph_p, &copied_min_ph_p))
				{
					double64 *copied_max_ph_p = NULL;

					if (CopyValidReal (max_ph_p, &copied_max_ph_p))
						{
							Location *location_p = (Location *) AllocMemory (sizeof (Location));

							if (location_p)
								{
									location_p -> lo_address_p = address_p;
									location_p -> lo_order = order;
									location_p -> lo_id_p = id_p;

									location_p -> lo_soil_s = copied_soil_s;
									location_p -> lo_min_ph_p = copied_min_ph_p;
									location_p -> lo_max_ph_p = copied_max_ph_p;

									location_p -> lo_type = loc_type;

									return location_p;
								}		/* if (location_p) */

							if (copied_max_ph_p)
								{
									FreeMemory (copied_max_ph_p);
								}
						}		/* if (CopyValidReal (max_ph_p, &copied_max_ph_p)) */

					if (copied_min_ph_p)
						{
							FreeMemory (copied_min_ph_p);
						}

				}		/* if (CopyValidReal (min_ph_p, &copied_min_ph_p)) */

			if (copied_soil_s)
				{
					FreeCopiedString (copied_soil_s);
				}

		}		/* if (CloneValidString (soil_s, &copied_soil_s)) */

	return NULL;
}


void FreeLocation (Location *location_p)
{
	if (location_p -> lo_address_p)
		{
			FreeAddress (location_p -> lo_address_p);
		}

	if (location_p -> lo_id_p)
		{
			FreeBSONOid (location_p -> lo_id_p);
		}

	if (location_p -> lo_soil_s)
		{
			FreeCopiedString (location_p -> lo_soil_s);
		}

	if (location_p -> lo_min_ph_p)
		{
			FreeMemory (location_p -> lo_min_ph_p);
		}

	if (location_p -> lo_max_ph_p)
		{
			FreeMemory (location_p -> lo_max_ph_p);
		}

	FreeMemory (location_p);
}


Location *CopyLocation (const Location * const src_p)
{
	bson_oid_t *id_p = GetNewBSONOid ();

	if (id_p)
		{
			Address *address_p = CopyAddress (src_p -> lo_address_p);

			if (address_p)
				{
					Location *location_p = NULL;

					bson_oid_copy (src_p -> lo_id_p, id_p);


					location_p = AllocateLocation (address_p, src_p -> lo_order, src_p -> lo_soil_s, src_p -> lo_min_ph_p, src_p -> lo_max_ph_p,
																				 src_p -> lo_type, id_p);

					if (location_p)
						{
							return location_p;
						}
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "AllocateLocation () failed when copying location \"%s\"", (src_p -> lo_address_p) ? src_p -> lo_address_p -> ad_name_s : "UNKNOWN");
						}


					FreeAddress (address_p);
				}
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "CopyAddress () failed when copying location \"%s\"", (src_p -> lo_address_p) ? src_p -> lo_address_p -> ad_name_s : "UNKNOWN");
				}


			FreeBSONOid (id_p);
		}
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "GetNewBSONOid () failed when copying location \"%s\"", (src_p -> lo_address_p) ? src_p -> lo_address_p -> ad_name_s : "UNKNOWN");
		}

	return NULL;
}



LocationNode *AllocateLocationNode (Location *location_p)
{
	LocationNode *node_p = (LocationNode *) AllocMemory (sizeof (LocationNode));

	if (node_p)
		{
			InitListItem (& (node_p -> ln_node));
			node_p -> ln_location_p = location_p;
		}

	return node_p;
}


void FreeLocationNode (ListItem *node_p)
{
	LocationNode *location_node_p = (LocationNode *) node_p;

	if (location_node_p -> ln_location_p)
		{
			FreeLocation (location_node_p -> ln_location_p);
		}

	FreeMemory (location_node_p);
}


json_t *GetLocationAsJSON (Location *location_p)
{
	json_t *location_json_p = json_object ();

	if (location_json_p)
		{
			if (AddCompoundIdToJSON (location_json_p, location_p -> lo_id_p))
				{
					if (SetJSONInteger (location_json_p, LO_ORDER_S, location_p -> lo_order))
						{
							/* if the name exists, add it to the top level for easier searching */
							if ( (! (location_p -> lo_address_p -> ad_name_s)) || (SetJSONString (location_json_p, LO_NAME_S, location_p -> lo_address_p -> ad_name_s)))
								{
									json_t *address_json_p = GetAddressAsJSON (location_p -> lo_address_p);

									if (address_json_p)
										{
											if (json_object_set_new (location_json_p, LO_ADDRESS_S, address_json_p) == 0)
												{

													if (SetNonTrivialDouble (location_json_p, LO_MIN_PH_S, location_p -> lo_min_ph_p, true))
														{
															if (SetNonTrivialDouble (location_json_p, LO_MAX_PH_S, location_p -> lo_max_ph_p, true))
																{
																	if (SetNonTrivialString (location_json_p, LO_SOIL_S, location_p -> lo_soil_s, true))
																		{
																			bool set_type_flag = false;

																			switch (location_p -> lo_type)
																				{
																					case LT_UNKNOWN:
																						set_type_flag = true;
																						break;

																					case LT_FARM:
																						set_type_flag = SetJSONString (location_json_p, LO_TYPE_S, LT_FARM_S);
																						break;

																					case LT_SITE:
																						set_type_flag = SetJSONString (location_json_p, LO_TYPE_S, LT_SITE_S);
																						break;

																					default:
																						break;
																				}		/* switch (location_p -> lo_type) */

																			if (set_type_flag)
																				{
																					if (AddDatatype (location_json_p, DFTD_LOCATION))
																						{
																							return location_json_p;
																						}
																				}
																			else
																				{
																					PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, location_json_p, "Failed to add location type %d", location_p -> lo_type);
																				}
																		}
																}
														}



												}		/* if (json_object_set_new (location_json_p, LO_ADDRESS_S, address_json_p) == 0) */
											else
												{
													json_decref (address_json_p);
												}

										}		/* if (address_json_p) */

								}		/*if ( (! (location_p -> lo_address_p -> ad_name_s)) || (SetJSONString (location_json_p, LO_NAME_S, location_p -> lo_address_p -> ad_name_s))) */

						}		/* if (SetJSONInteger (location_json_p, LO_ORDER_S, location_p -> lo_order)) */

				}		/* if (AddCompoundIdToJSON (location_json_p, location_p -> lo_id_p)) */

			json_decref (location_json_p);
		}		/* if (location_json_p) */

	return NULL;
}


Location *GetLocationFromJSON (const json_t *location_json_p, const FieldTrialServiceData * UNUSED_PARAM (data_p))
{
	uint32 order;

	if (GetJSONUnsignedInteger (location_json_p, LO_ORDER_S, &order))
		{
			bson_oid_t *id_p = GetNewUnitialisedBSONOid ();

			if (id_p)
				{
					if (GetMongoIdFromJSON (location_json_p, id_p))
						{
							const json_t *address_json_p = json_object_get (location_json_p, LO_ADDRESS_S);

							if (address_json_p)
								{
									Address *address_p = GetAddressFromJSON (address_json_p);

									if (address_p)
										{
											Location *location_p = NULL;
											double64 *min_ph_p = NULL;
											double64 *max_ph_p = NULL;
											LocationType loc_type = LT_UNKNOWN;
											const char *soil_s = GetJSONString (location_json_p, LO_SOIL_S);
											const char *loc_type_s = GetJSONString (location_json_p, LO_TYPE_S);

											GetValidRealFromJSON (location_json_p, LO_MIN_PH_S, &min_ph_p);
											GetValidRealFromJSON (location_json_p, LO_MAX_PH_S, &max_ph_p);


											if (loc_type_s)
												{
													if (strcmp (loc_type_s, LT_FARM_S) == 0)
														{
															loc_type = LT_FARM;
														}
													else if (strcmp (loc_type_s, LT_SITE_S) == 0)
														{
															loc_type = LT_SITE;
														}
												}


											location_p = AllocateLocation (address_p, order, soil_s, min_ph_p, max_ph_p, loc_type, id_p);

											if (location_p)
												{
													return location_p;
												}

											FreeAddress (address_p);
										}		/* if (address_p) */


								}		/* if (address_json_p) */

						}		/* if (GetCompoundIdFromJSON (location_json_p, id_p)) */

					FreeBSONOid (id_p);
				}		/* if (id_p) */

		}		/* if (GetJSONInteger (location_json_p, LO_ORDER_S, (int *) &order)) */

	return NULL;
}




OperationStatus SaveLocation (Location *location_p, ServiceJob *job_p, FieldTrialServiceData *data_p)
{
	OperationStatus status = OS_FAILED;
	bson_t *selector_p = NULL;
	bool success_flag = PrepareSaveData (& (location_p -> lo_id_p), &selector_p);

	if (success_flag)
		{
			json_t *location_json_p = GetLocationAsJSON (location_p);

			if (location_json_p)
				{
					MongoTool *mongo_p = GetConfiguredMongoTool (data_p, NULL);

					if (mongo_p)
						{
							if (SaveAndBackupMongoDataWithTimestamp (mongo_p, location_json_p, data_p -> dftsd_collection_ss [DFTD_LOCATION],
									data_p -> dftsd_backup_collection_ss [DFTD_LOCATION], DFT_BACKUPS_ID_KEY_S, selector_p, MONGO_TIMESTAMP_S))
								{
									status = IndexData (job_p, location_json_p, NULL);

									if (status != OS_SUCCEEDED)
										{
											status = OS_PARTIALLY_SUCCEEDED;
											PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, location_json_p, "Failed to index location \"%s\" as JSON to Lucene", location_p -> lo_address_p -> ad_name_s);
											AddGeneralErrorMessageToServiceJob (job_p, "Location saved but failed to index for searching");
										}
								}


							FreeMongoTool (mongo_p);
						}		/* if (mongo_p) */
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "GetConfiguredMongoTool () failed");
						}




					json_decref (location_json_p);
				}		/* if (area_json_p) */
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to get location \"%s\" as JSON", location_p -> lo_address_p -> ad_name_s);
					success_flag = false;
				}

		}		/* if (location_p -> lo_id_p) */

	SetServiceJobStatus (job_p, status);

	return status;
}



Location *GetLocationById (bson_oid_t *id_p, const ViewFormat format, const FieldTrialServiceData *data_p)
{
	Location *location_p = GetDFWObjectById (id_p, DFTD_LOCATION, GetLocationFromJSONCallback, format, data_p);

	return location_p;
}


Location *GetLocationByIdString (const char *location_id_s, const ViewFormat format, const FieldTrialServiceData *data_p)
{
	Location *location_p = GetDFWObjectByIdString (location_id_s, DFTD_LOCATION, GetLocationFromJSONCallback, format, data_p);

	return location_p;
}





/*
 * The search string could be the bson_oid or a name so check
 */

Location *GetUniqueLocationBySearchString (const char *location_s, const ViewFormat format, const FieldTrialServiceData *data_p)
{
	Location *location_p = NULL;

	if (bson_oid_is_valid (location_s, strlen (location_s)))
		{
			location_p = GetLocationByIdString (location_s, format, data_p);
		}

	if (!location_p)
		{
			const char *keys_ss [2];
			const char *values_ss [2];

			* (keys_ss + 0) = LO_NAME_S;
			* (keys_ss + 1) = NULL;

			* (values_ss + 0) = location_s;
			* (values_ss + 1) = NULL;


			LinkedList *locations_p = SearchObjects (data_p, DFTD_LOCATION, keys_ss, values_ss, FreeLocationNode, AddLocationResultToList);

			if (locations_p)
				{
					if (locations_p -> ll_size == 1)
						{
							LocationNode *node_p = (LocationNode *) (locations_p -> ll_head_p);

							/* Remove the location from the node */
							location_p = node_p -> ln_location_p;
							node_p -> ln_location_p = NULL;
						}

					FreeLinkedList (locations_p);
				}
		}


	return location_p;
}


static bool AddLocationResultToList (const json_t *location_json_p, LinkedList *locations_p, const FieldTrialServiceData *service_data_p)
{
	bool success_flag = false;
	Location *location_p = GetLocationFromJSON (location_json_p, service_data_p);

	if (location_p)
		{
			LocationNode *node_p = AllocateLocationNode (location_p);

			if (node_p)
				{
					LinkedListAddTail (locations_p, & (node_p -> ln_node));
					success_flag = true;
				}

		}		/* if (location_p) */
	else
		{
			PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, location_json_p, "Failed to create Location");
		}

	return success_flag;
}


char *GetLocationAsString (const Location *location_p)
{
	char *location_s = NULL;

	if (location_p -> lo_address_p)
		{
			location_s = GetAddressAsString (location_p -> lo_address_p);
		}

	return location_s;
}


static void *GetLocationFromJSONCallback (const json_t *location_json_p, const ViewFormat format, const FieldTrialServiceData *data_p)
{
	return GetLocationFromJSON (location_json_p, data_p);
}



const char *GetLocationTypeAsString (const LocationType loc_type)
{
	const char *loc_s = NULL;

	switch (loc_type)
		{
			case LT_FARM:
				loc_s = LT_FARM_S;
				break;

			case LT_SITE:
				loc_s = LT_SITE_S;
				break;

			default:
				break;
		}

	return loc_s;
}


bool GetLocationTypeFromString (const char *loc_type_s, LocationType *loc_type_p)
{
	bool success_flag = false;

	if (loc_type_s)
		{
			if (strcmp (loc_type_s, LT_FARM_S) == 0)
				{
					*loc_type_p = LT_FARM;
					success_flag = true;
				}
			else if (strcmp (loc_type_s, LT_SITE_S) == 0)
				{
					*loc_type_p = LT_SITE;
					success_flag = true;
				}

		}		/* if (loc_type_s) */

	return success_flag;
}


