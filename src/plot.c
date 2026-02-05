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
 * plot.c
 *
 *  Created on: 28 Sep 2018
 *      Author: billy
 */


#define ALLOCATE_PLOT_TAGS (1)
#include "plot.h"
#include "memory_allocations.h"
#include "string_utils.h"
#include "standard_row.h"
#include "dfw_util.h"
#include "time_util.h"
#include "study.h"
#include "int_linked_list.h"
#include "mongodb_util.h"


static bool AddRowsToJSON (const Plot *plot_p, json_t *plot_json_p, const ViewFormat format, JSONProcessor *processor_p, const FieldTrialServiceData *data_p);

static bool SetValidPlotImage (const Plot *plot_p, json_t *plot_json_p);




Plot *AllocatePlot (bson_oid_t *id_p, const struct tm *sowing_date_p, const struct tm *harvest_date_p, const double64 *width_p, const double64 *length_p, const uint32 row_index,
										const uint32 column_index, const char *treatments_s, const char *comment_s, const char *image_s, const char *thumbnail_s,
										const uint32 *sowing_order_p, const uint32 *walking_order_p, Study *parent_p)
{
	char *copied_treatments_s = NULL;

	if ((IsStringEmpty (treatments_s)) || ((copied_treatments_s = EasyCopyToNewString (treatments_s)) != NULL))
		{
			char *copied_comment_s = NULL;

			if ((IsStringEmpty (comment_s)) || ((copied_comment_s = EasyCopyToNewString (comment_s)) != NULL))
				{
					char *copied_image_s = NULL;

					if ((IsStringEmpty (image_s)) || ((copied_image_s = EasyCopyToNewString (image_s)) != NULL))
						{
							char *copied_thumbnail_s = NULL;

							if ((IsStringEmpty (thumbnail_s)) || ((copied_thumbnail_s = EasyCopyToNewString (thumbnail_s)) != NULL))
								{
									struct tm *copied_sowing_date_p = NULL;

									if (CopyValidDate (sowing_date_p, &copied_sowing_date_p))
										{
											struct tm *copied_harvest_date_p = NULL;

											if (CopyValidDate (harvest_date_p, &copied_harvest_date_p))
												{
													double64 *copied_width_p = NULL;

													if (CopyValidReal (width_p, &copied_width_p))
														{
															double64 *copied_length_p = NULL;

															if (CopyValidReal (length_p, &copied_length_p))
																{
																	uint32 *copied_sowing_order_p = NULL;

																	if (CopyValidUnsignedInteger (sowing_order_p, &copied_sowing_order_p))
																		{
																			uint32 *copied_walking_order_p = NULL;

																			if (CopyValidUnsignedInteger (walking_order_p, &copied_walking_order_p))
																				{
																					LinkedList *rows_p = AllocateLinkedList (FreeRowNode);

																					if (rows_p)
																						{
																							Plot *plot_p = (Plot *) AllocMemory (sizeof (Plot));

																							if (plot_p)
																								{
																									plot_p -> pl_id_p = id_p;
																									plot_p -> pl_sowing_date_p = copied_sowing_date_p;
																									plot_p -> pl_harvest_date_p = copied_harvest_date_p;
																									plot_p -> pl_width_p = copied_width_p;
																									plot_p -> pl_length_p = copied_length_p;
																									plot_p -> pl_row_index = row_index;
																									plot_p -> pl_column_index = column_index;

																									plot_p -> pl_parent_p = parent_p;
																									plot_p -> pl_rows_p = rows_p;

																									plot_p -> pl_comment_s = copied_comment_s;

																									plot_p -> pl_treatments_s = copied_treatments_s;

																									plot_p -> pl_soil_type_s = NULL;
																									plot_p -> pl_sowing_rate = 0.0f;

																									plot_p -> pl_image_url_s = copied_image_s;

																									plot_p -> pl_thumbnail_url_s = copied_thumbnail_s;

																									plot_p -> pl_sowing_order_index_p = copied_sowing_order_p;
																									plot_p -> pl_walking_order_index_p = copied_walking_order_p;

																									return plot_p;
																								}		/* if (plot_p) */

																							FreeLinkedList (rows_p);
																						}		/* if (rows_p) */


																					if (copied_walking_order_p)
																						{
																							FreeMemory (copied_walking_order_p);
																						}
																				}		/* if (CopyValidUnsignedInteger (harvest_order_p, &copied_harvest_order_p)) */
																			else
																				{
																					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to copy walking order %lu\n", walking_order_p ? *walking_order_p : 0);
																				}



																			if (copied_sowing_order_p)
																				{
																					FreeMemory (copied_sowing_order_p);
																				}
																		}		/* if (CopyValidUnsignedInteger (sowing_order_p, &copied_sowing_order_p)) */
																	else
																		{
																			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to copy sowing order %lu\n", sowing_order_p ? *sowing_order_p : 0);
																		}


																	if (copied_length_p)
																		{
																			FreeMemory (copied_length_p);
																		}

																}		/* if (CopyValidReal (length_p, &copied_length_p)) */


															if (copied_width_p)
																{
																	FreeMemory (copied_width_p);
																}

														}		/* if (CopyValidReal (width_p, &copied_width_p)) */


													if (copied_harvest_date_p)
														{
															FreeTime (copied_harvest_date_p);
														}

												}		/* if (CopyValidDate (harvest_date_p, &copied_harvest_date_p)) */

											if (copied_sowing_date_p)
												{
													FreeTime (copied_sowing_date_p);
												}

										}		/* if (CopyValidDate (sowing_date_p, &copied_sowing_date_p)) */

									if (copied_thumbnail_s)
										{
											FreeCopiedString (copied_thumbnail_s);
										}

								}		/* if ((IsStringEmpty (thumbnail_s)) || ((copied_thumbnail_s = EasyCopyToNewString (thumbnail_s)) != NULL)) */


							if (copied_image_s)
								{
									FreeCopiedString (copied_image_s);
								}
						}

					if (copied_comment_s)
						{
							FreeCopiedString (copied_comment_s);
						}


				}		/* if ((IsStringEmpty (comment_s)) || ((copied_comment_s = EasyCopyToNewString (comment_s)) != NULL)) */


			if (copied_treatments_s)
				{
					FreeCopiedString (copied_treatments_s);
				}

		}		/* if ((IsStringEmpty (treatments_s)) || ((copied_treatments_s = EasyCopyToNewString (treatments_s)) != NULL)) */

	return NULL;
}




void FreePlot (Plot *plot_p)
{
	if (plot_p -> pl_id_p)
		{
			FreeBSONOid (plot_p -> pl_id_p);
		}


	if (plot_p -> pl_comment_s)
		{
			FreeCopiedString (plot_p -> pl_comment_s);
		}

	if (plot_p -> pl_treatments_s)
		{
			FreeCopiedString (plot_p -> pl_treatments_s);
		}

	if (plot_p -> pl_image_url_s)
		{
			FreeCopiedString (plot_p -> pl_image_url_s);
		}

	if (plot_p -> pl_thumbnail_url_s)
		{
			FreeCopiedString (plot_p -> pl_thumbnail_url_s);
		}

	FreeLinkedList (plot_p -> pl_rows_p);

	if (plot_p -> pl_sowing_date_p)
		{
			FreeTime (plot_p -> pl_sowing_date_p);
		}


	if (plot_p -> pl_harvest_date_p)
		{
			FreeTime (plot_p -> pl_harvest_date_p);
		}

	if (plot_p -> pl_width_p)
		{
			FreeMemory (plot_p -> pl_width_p);
		}

	if (plot_p -> pl_length_p)
		{
			FreeMemory (plot_p -> pl_length_p);
		}


	if (plot_p -> pl_sowing_order_index_p)
		{
			FreeMemory (plot_p -> pl_sowing_order_index_p);
		}

	if (plot_p -> pl_walking_order_index_p)
		{
			FreeMemory (plot_p -> pl_walking_order_index_p);
		}

	FreeMemory (plot_p);
}


PlotNode *AllocatePlotNode (Plot *plot_p)
{
	PlotNode *pl_node_p = (PlotNode *) AllocMemory (sizeof (PlotNode));

	if (pl_node_p)
		{
			InitListItem (& (pl_node_p -> pn_node));

			pl_node_p -> pn_plot_p = plot_p;
		}

	return pl_node_p;
}



void FreePlotNode (ListItem *node_p)
{
	PlotNode *pl_node_p = (PlotNode *) node_p;

	if (pl_node_p -> pn_plot_p)
		{
			FreePlot (pl_node_p -> pn_plot_p);
		}

	FreeMemory (pl_node_p);
}


bool SavePlot (Plot *plot_p, const FieldTrialServiceData *data_p)
{
	bson_t *selector_p = NULL;
	bool success_flag = PrepareSaveData (& (plot_p -> pl_id_p), &selector_p);

	if (success_flag)
		{
			json_t *plot_json_p = GetPlotAsJSON (plot_p, VF_STORAGE, NULL, data_p);

			if (plot_json_p)
				{
					MongoTool *mongo_p = GetConfiguredMongoTool (data_p, NULL);

					if (mongo_p)
						{
							success_flag = SaveAndBackupMongoDataWithTimestamp (mongo_p, plot_json_p, data_p -> dftsd_collection_ss [DFTD_PLOT],
							data_p -> dftsd_backup_collection_ss [DFTD_PLOT], DFT_BACKUPS_ID_KEY_S, selector_p, MONGO_TIMESTAMP_S);

							FreeMongoTool (mongo_p);
						}		/* if (mongo_p) */
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "GetConfiguredMongoTool () failed");
						}


					json_decref (plot_json_p);
				}		/* if (plot_json_p) */


			if (selector_p)
				{
					bson_free (selector_p);
				}

		}		/* if (plot_p -> pl_id_p) */

	return success_flag;
}


json_t *GetPlotAsJSON (Plot *plot_p, const ViewFormat format, JSONProcessor *processor_p, const FieldTrialServiceData *data_p)
{
	json_t *plot_json_p = json_object ();

	if (plot_json_p)
		{
			if (SetJSONInteger (plot_json_p, PL_ROW_INDEX_S, plot_p -> pl_row_index))
				{
					if (SetJSONInteger (plot_json_p, PL_COLUMN_INDEX_S, plot_p -> pl_column_index))
						{
							if (SetNonTrivialDouble (plot_json_p, PL_WIDTH_S, plot_p -> pl_width_p, true))
								{
									if (SetNonTrivialDouble (plot_json_p, PL_LENGTH_S, plot_p -> pl_length_p, true))
										{
											if (SetNonTrivialUnsignedInt (plot_json_p, PL_SOWING_ORDER_S, plot_p -> pl_sowing_order_index_p, true))
												{
													if (SetNonTrivialUnsignedInt (plot_json_p, PL_WALKING_ORDER_S, plot_p -> pl_walking_order_index_p, true))
														{
															if (SetNonTrivialString (plot_json_p, PL_COMMENT_S, plot_p -> pl_comment_s, false))
																{
																	if (SetNonTrivialString (plot_json_p, PL_TREATMENT_S, plot_p -> pl_treatments_s, false))
																		{
																			if (SetNonTrivialString (plot_json_p, PL_IMAGE_S, plot_p -> pl_image_url_s, false))
																				{
																					if (AddValidDateToJSON (plot_p -> pl_sowing_date_p, plot_json_p, PL_SOWING_DATE_S, false))
																						{
																							if (AddValidDateToJSON (plot_p -> pl_harvest_date_p, plot_json_p, PL_HARVEST_DATE_S, false))
																								{
																									bool success_flag = false;

																									switch (format)
																										{
																											case VF_CLIENT_MINIMAL:
																												{
																													success_flag = SetValidPlotImage (plot_p, plot_json_p);
																												}
																												break;


																											case VF_CLIENT_FULL:
																												{
																													/*
																													 * The django fontend uses the plot id so we need to send it
																													 */
																													if (AddCompoundIdToJSON (plot_json_p, plot_p -> pl_id_p))
																														{
																															if (SetValidPlotImage (plot_p, plot_json_p))
																															{
																																if (AddRowsToJSON (plot_p, plot_json_p, format, processor_p, data_p))
																																	{
																																		success_flag = true;
																																	}
																															}
																														}
																												}		/* case VF_CLIENT_FULL: */
																												break;

																											case VF_STORAGE:
																												{
																													if (AddCompoundIdToJSON (plot_json_p, plot_p -> pl_id_p))
																														{
																															if (AddNamedCompoundIdToJSON (plot_json_p, plot_p -> pl_parent_p -> st_id_p, PL_PARENT_STUDY_S))
																																{
																																	if (SetJSONString (plot_json_p, PL_IMAGE_S, plot_p -> pl_image_url_s))
																																		{
																																			if (SetJSONString (plot_json_p, PL_THUMBNAIL_S, plot_p -> pl_thumbnail_url_s))
																																				{
																																					if (AddRowsToJSON (plot_p, plot_json_p, format, processor_p, data_p))
																																						{
																																							success_flag = true;
																																						}
																																				}
																																		}
																																}		/* if (AddiNamedCompoundIdToJSON (plot_json_p, plot_p -> pl_parent_p -> st_id_p, PL_PARENT_FIELD_TRIAL_S)) */
																															else
																																{
																																	PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, plot_json_p, "Failed to add id for \"%s\"", PL_PARENT_STUDY_S);
																																}

																														}		/* if (AddCompoundIdToJSON (plot_json_p, plot_p -> pl_id_p)) */
																													else
																														{
																															PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, plot_json_p, "Failed to add id");
																														}


																												}		/* case VF_STORAGE */
																												break;

																											default:
																												break;

																										}		/* switch (format) */

																									if (success_flag)
																										{
																											if (AddDatatype (plot_json_p, DFTD_PLOT))
																												{
																													return plot_json_p;
																												}
																										}



																								}		/* if (AddValidDateToJSON (plot_p -> pl_harvest_date_p, plot_json_p, PL_HARVEST_DATE_S)) */
																							else
																								{
																									char *time_s = NULL;

																									if (plot_p -> pl_harvest_date_p)
																										{
																											time_s = GetTimeAsString (plot_p -> pl_harvest_date_p, false, NULL);
																										}

																									PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, plot_json_p, "Failed to add \"%s\": \"%s\"", PL_HARVEST_DATE_S, time_s ? time_s : "");

																									if (time_s)
																										{
																											FreeCopiedString (time_s);
																										}
																								}

																						}		/* if (AddValidDateToJSON (plot_p -> pl_sowing_date_p, plot_json_p, PL_SOWING_DATE_S)) */
																					else
																						{
																							char *time_s = NULL;

																							if (plot_p -> pl_sowing_date_p)
																								{
																									time_s = GetTimeAsString (plot_p -> pl_sowing_date_p, false, NULL);
																								}

																							PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, plot_json_p, "Failed to add \"%s\": \"%s\"", PL_SOWING_DATE_S, time_s ? time_s : "");

																							if (time_s)
																								{
																									FreeCopiedString (time_s);
																								}
																						}

																				}		/* if ((IsStringEmpty (plot_p -> pl_link_s)) || (SetJSONString (plot_json_p, PL_URL_S, plot_p -> pl_link_s))) */
																			else
																				{
																					PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, plot_json_p, "Failed to add \"%s\": \"%s\"", PL_IMAGE_S, plot_p -> pl_image_url_s);
																				}


																		}		/* if ((IsStringEmpty (plot_p -> pl_treatments_s)) || (SetJSONString (plot_json_p, PL_COMMENT_S, plot_p -> pl_treatments_s))) */
																	else
																		{
																			PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, plot_json_p, "Failed to add \"%s\": \"%s\"", PL_TREATMENT_S, plot_p -> pl_comment_s);
																		}

																}		/* if ((IsStringEmpty (plot_p -> pl_comment_s)) || (SetJSONString (plot_json_p, PL_COMMENT_S, plot_p -> pl_comment_s))) */
															else
																{
																	PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, plot_json_p, "Failed to add \"%s\": \"%s\"", PL_COMMENT_S, plot_p -> pl_comment_s);
																}

														}		/* if (SetNonTrivialUnsignedInt (plot_json_p, PL_WALKING_ORDER_S, plot_p -> pl_walking_order_index_p)) */
													else
														{
															if (plot_p -> pl_walking_order_index_p)
																{
																	PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, plot_json_p, "Failed to add \"%s\": "  UINT32_FMT "", PL_WALKING_ORDER_S, * (plot_p -> pl_walking_order_index_p));
																}
															else
																{
																	PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, plot_json_p, "Failed to add \"%s\": null", PL_WALKING_ORDER_S);
																}
														}

												}		/* if (SetNonTrivialUnsignedInt (plot_json_p, PL_SOWING_ORDER_S, plot_p -> pl_sowing_order_index_p)) */
											else
												{
													if (plot_p -> pl_sowing_order_index_p)
														{
															PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, plot_json_p, "Failed to add \"%s\": " UINT32_FMT, PL_SOWING_ORDER_S, * (plot_p -> pl_sowing_order_index_p));
														}
													else
														{
															PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, plot_json_p, "Failed to add \"%s\": null", PL_SOWING_ORDER_S);
														}
												}


										}		/* if (SetJSONReal (plot_json_p, PL_LENGTH_S, plot_p -> pl_length)) */
									else
										{
											if (plot_p -> pl_length_p)
												{
													PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, plot_json_p, "Failed to add \"%s\": " DOUBLE64_FMT, PL_LENGTH_S, * (plot_p -> pl_length_p));
												}
											else
												{
													PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, plot_json_p, "Failed to add \"%s\": null", PL_LENGTH_S);
												}
										}

								}		/* if (SetJSONReal (plot_json_p, PL_WIDTH_S, plot_p -> pl_width)) */
							else
								{
									if (plot_p -> pl_width_p)
										{
											PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, plot_json_p, "Failed to add \"%s\": " DOUBLE64_FMT, PL_WIDTH_S, * (plot_p -> pl_width_p));
										}
									else
										{
											PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, plot_json_p, "Failed to add \"%s\": null", PL_WIDTH_S);
										}

								}

						}		/* if (SetJSONInteger (plot_json_p, PL_COLUMN_INDEX_S, plot_p -> pl_column_index)) */
					else
						{
							PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, plot_json_p, "Failed to add \"%s\": " UINT32_FMT, PL_COLUMN_INDEX_S, plot_p -> pl_column_index);
						}

				}		/* if (SetJSONInteger (plot_json_p, PL_ROW_INDEX_S, plot_p -> pl_row_index)) */
			else
				{
					PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, plot_json_p, "Failed to add \"%s\": " UINT32_FMT, PL_ROW_INDEX_S, plot_p -> pl_row_index);
				}

			json_decref (plot_json_p);
		}		/* if (plot_json_p) */

	return NULL;
}



Plot *GetPlotFromJSON (const json_t *plot_json_p, Study *parent_study_p, const ViewFormat format, FieldTrialServiceData *data_p)
{
	Plot *plot_p = NULL;
	json_int_t row;

	if (GetJSONInteger (plot_json_p, PL_ROW_INDEX_S, &row))
		{
			json_int_t column;

			if (GetJSONInteger (plot_json_p, PL_COLUMN_INDEX_S, &column))
				{
					const char *treatments_s = GetJSONString (plot_json_p, PL_TREATMENT_S);
					const char *comment_s = GetJSONString (plot_json_p, PL_COMMENT_S);
					const char *image_s = GetJSONString (plot_json_p, PL_IMAGE_S);
					const char *thumbnail_s = GetJSONString (plot_json_p, PL_THUMBNAIL_S);
					double64 *width_p = NULL;
					double64 *length_p = NULL;
					uint32 *sowing_order_p = NULL;
					uint32 *walking_order_p = NULL;
					struct tm *sowing_date_p = NULL;

					GetValidRealFromJSON (plot_json_p, PL_WIDTH_S, &width_p);
					GetValidRealFromJSON (plot_json_p, PL_LENGTH_S, &length_p);

					GetValidUnsignedIntFromJSON (plot_json_p, PL_SOWING_ORDER_S, &sowing_order_p);
					GetValidUnsignedIntFromJSON (plot_json_p, PL_WALKING_ORDER_S, &walking_order_p);


					if (CreateValidDateFromJSON (plot_json_p, PL_SOWING_DATE_S, &sowing_date_p))
						{
							struct tm *harvest_date_p = NULL;

							if (CreateValidDateFromJSON (plot_json_p, PL_HARVEST_DATE_S, &harvest_date_p))
								{
									bson_oid_t *id_p = GetNewUnitialisedBSONOid ();

									if (id_p)
										{
											if (GetMongoIdFromJSON (plot_json_p, id_p))
												{
													if (!parent_study_p)
														{
															bson_oid_t *parent_study_id_p = GetNewUnitialisedBSONOid ();

															if (parent_study_id_p)
																{
																	if (GetNamedIdFromJSON (plot_json_p, PL_PARENT_STUDY_S, parent_study_id_p))
																		{
																			parent_study_p = GetStudyById (parent_study_id_p, VF_CLIENT_MINIMAL, data_p);

																			if (!parent_study_p)
																				{
																					char *parent_study_id_s = GetBSONOidAsString (parent_study_id_p);

																					if (parent_study_id_s)
																						{
																							PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, plot_json_p, "Failed to get parent study with id \"%s\"", parent_study_id_s);

																							FreeBSONOidString (parent_study_id_s);
																						}
																					else
																						{
																							PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, plot_json_p, "Failed to get parent study");
																						}
																				}

																		}		/* if (GetNamedIdFromJSON (plot_json_p, PL_PARENT_FIELD_TRIAL_S, field_trial_id_p)) */
																	else
																		{
																			PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, plot_json_p, "Failed to get id for \"%s\"", PL_PARENT_STUDY_S);
																		}

																	FreeBSONOid (parent_study_id_p);
																}		/* if (parent_study_id_p) */
															else
																{
																	PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate id for \"%s\"", PL_PARENT_STUDY_S);
																}

														}		/* if (!parent_study_p) */

													plot_p = AllocatePlot (id_p, sowing_date_p, harvest_date_p, width_p, length_p, row, column, treatments_s, comment_s, image_s, thumbnail_s,
																								 sowing_order_p, walking_order_p, parent_study_p);

													if (plot_p)
														{
															json_t *rows_array_p = json_object_get (plot_json_p, PL_ROWS_S);

															if (rows_array_p)
																{
																	if (!GetPlotRows (plot_p, rows_array_p, parent_study_p, format, data_p))
																		{
																			PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, plot_json_p, "GetPlotRows () failed for format %d in study \"%s\"", format, parent_study_p ? parent_study_p -> st_name_s : "NULL");

																		}
																}
														}
													else
														{
															PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, plot_json_p, "Failed to get create Plot");
														}

												}		/* if (GetMongoIdFromJSON (plot_json_p, id_p)) */
											else
												{
													PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, plot_json_p, "Failed to get id for \"%s\"", MONGO_ID_S);
												}

										}		/* if (id_p) */
									else
										{
											PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate id for \"%s\"", MONGO_ID_S);
										}

									if (harvest_date_p)
										{
											FreeTime (harvest_date_p);
										}

								}		/* if (CreateValidDateFromJSON (plot_json_p, PL_HARVEST_DATE_S, &harvest_date_p)) */
							else
								{
									PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, plot_json_p, "Failed to get time from \"%s\"", PL_HARVEST_DATE_S);
								}

							if (sowing_date_p)
								{
									FreeTime (sowing_date_p);
								}

						}		/* if (CreateValidDateFromJSON (plot_json_p, PL_SOWING_DATE_S, &sowing_date_p)) */
					else
						{
							PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, plot_json_p, "Failed to get time from \"%s\"", PL_HARVEST_DATE_S);
						}


					if (width_p)
						{
							FreeMemory (width_p);
						}


					if (length_p)
						{
							FreeMemory (length_p);
						}

				}		/* if (GetJSONInteger (plot_json_p, PL_COLUMN_INDEX_S, plot_p -> pl_column_index)) */
			else
				{
					PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, plot_json_p, "Failed to get \"%s\": ", PL_COLUMN_INDEX_S);
				}

		}		/* if (GetJSONInteger (plot_json_p, PL_ROW_INDEX_S, &row)) */
	else
		{
			PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, plot_json_p, "Failed to get \"%s\": ", PL_ROW_INDEX_S);
		}



	return plot_p;
}


Row *GetRowFromPlotByStudyIndex (Plot *plot_p, const uint32 by_study_index)
{
	RowNode *node_p = (RowNode *) (plot_p -> pl_rows_p -> ll_head_p);

	while (node_p)
		{
			Row *row_p = node_p -> rn_row_p;

			if (row_p -> ro_by_study_index == by_study_index)
				{
					return row_p;
				}

			node_p = (RowNode *) (node_p -> rn_node.ln_next_p);
		}

	return NULL;
}


Row *GetRowFromPlotByRackIndex (Plot *plot_p, const uint32 rack_index)
{
	RowNode *node_p = (RowNode *) (plot_p -> pl_rows_p -> ll_head_p);

	while (node_p)
		{
			if (node_p -> rn_row_p -> ro_type == RT_STANDARD)
				{
					StandardRow *row_p = (StandardRow *) (node_p -> rn_row_p);

					if (row_p -> sr_rack_index == rack_index)
						{
							return (& (row_p -> sr_base));
						}

				}

			node_p = (RowNode *) (node_p -> rn_node.ln_next_p);
		}

	return NULL;
}


bool AddRowToPlot (Plot *plot_p, Row *row_p)
{
	bool success_flag = false;
	RowNode *node_p = AllocateRowNode (row_p);

	if (node_p)
		{
			LinkedListAddTail (plot_p -> pl_rows_p, & (node_p -> rn_node));
			success_flag = true;
		}
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__,  "Failed to add row to plot's list");
		}

	return success_flag;
}


bool GetPlotRows (Plot *plot_p, json_t *rows_array_p, const Study *study_p, const ViewFormat format, FieldTrialServiceData *data_p)
{
	bool success_flag = false;

	ClearLinkedList (plot_p -> pl_rows_p);
	size_t i;
	const size_t num_results = json_array_size (rows_array_p);

	success_flag = true;

	if (num_results > 0)
		{
			json_t *row_json_p;

			json_array_foreach (rows_array_p, i, row_json_p)
				{
					Row *row_p = GetRowFromJSON (row_json_p, plot_p, study_p, format, data_p);

					if (row_p)
						{
							if (AddRowToPlot (plot_p, row_p))
								{
									success_flag = true;
								}
							else
								{
									char id_s [MONGO_OID_STRING_BUFFER_SIZE];

									bson_oid_to_string (plot_p -> pl_id_p, id_s);
									PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, row_json_p, "AddRowToPlot () failed for plot with id \"%s\"", id_s);

									FreeRow (row_p);
								}

						}		/* if (row_p) */
					else
						{
							char id_s [MONGO_OID_STRING_BUFFER_SIZE];

							bson_oid_to_string (plot_p -> pl_id_p, id_s);
							PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, row_json_p, "Failed to get row from json for plot with id \"%s\"", id_s);
						}	
						
				}		/* json_array_foreach (results_p, i, entry_p) */

		}		/* if (num_results > 0) */
	else
		{
			char id_s [MONGO_OID_STRING_BUFFER_SIZE];

			bson_oid_to_string (plot_p -> pl_id_p, id_s);
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "No rows found for plot with id \"%s\"", id_s);
		}


	return success_flag;
}



Plot *GetPlotByIdString (const char *plot_id_s, const ViewFormat format, const FieldTrialServiceData *data_p)
{
	Plot *plot_p = GetDFWObjectByIdString (plot_id_s, DFTD_PLOT, GetPlotCallback, format, data_p);

	return plot_p;
}


void *GetPlotCallback (const json_t *json_p, const ViewFormat format, const FieldTrialServiceData *data_p)
{
	return GetPlotFromJSON (json_p, NULL, format, data_p);
}


Plot *AllocateSkeletonPlot (const int32 row, const int32 column, Study *study_p, const FieldTrialServiceData *data_p)
{
	return  AllocatePlot (NULL, NULL, NULL, NULL, NULL, row, column, NULL, NULL, NULL, NULL, NULL, NULL, study_p);
}



//Plot *GetPlotByIndex (const Study *study_p, const uint32 plot_index, const FieldTrialServiceData *data_p)
//{
//	Plot *plot_p = NULL;
//	bson_t *query_p = BCON_NEW (PL_INDEX_S, BCON_INT32 (plot_index), PL_PARENT_STUDY_S, BCON_OID (study_p -> st_id_p));
//
//	if (query_p)
//		{
//			plot_p = SearchForPlot (query_p, data_p);
//
//			if (!plot_p)
//				{
//					PrintBSONToErrors (STM_LEVEL_INFO, __FILE__, __LINE__, query_p, "SearchForPlot did not find plot " UINT32_FMT " in study \"%s\"", plot_index, study_p -> st_name_s);
//				}
//
//			bson_destroy (query_p);
//
//		}		/* if (query_p) */
//	else
//		{
//			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "GetPlotByIndex could not create query for plot " UINT32_FMT " in study \"%s\"", plot_index, study_p -> st_name_s);
//		}
//
//	return plot_p;
//}


static bool AddRowsToJSON (const Plot *plot_p, json_t *plot_json_p, const ViewFormat format, JSONProcessor *processor_p, const FieldTrialServiceData *data_p)
{
	bool success_flag = false;
	json_t *rows_json_p = json_array ();

	if (rows_json_p)
		{
			if (json_object_set_new (plot_json_p, PL_ROWS_S, rows_json_p) == 0)
				{
					RowNode *node_p = (RowNode *) (plot_p -> pl_rows_p -> ll_head_p);

					success_flag = true;

					while (node_p && success_flag)
						{
							Row *row_p = node_p -> rn_row_p;
							json_t *row_json_p = ProcessRowJSON (processor_p, row_p, format, data_p);

							if (row_json_p)
								{
									if (json_array_append_new (rows_json_p, row_json_p) == 0)
										{
											node_p = (RowNode *) (node_p -> rn_node.ln_next_p);
										}
									else
										{
											success_flag = false;
											PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, plot_json_p, "Failed to add row json to results");
										}
								}
							else
								{
									char id_s [MONGO_OID_STRING_BUFFER_SIZE];

									success_flag = false;
									bson_oid_to_string (node_p -> rn_row_p -> ro_id_p, id_s);

									PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, plot_json_p, "Failed to create row json for \"%s\"", id_s);
								}

						}		/* while (node_p && &success_flag) */

				}
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add rows array");
					json_decref (rows_json_p);
				}
		}
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to create plots array");
		}

	return success_flag;
}


static bool SetValidPlotImage (const Plot *plot_p, json_t *plot_json_p)
{
	bool success_flag = false;

	if (plot_p -> pl_image_url_s)
		{
			json_t *image_json_p = GetImageObject (plot_p -> pl_image_url_s, plot_p -> pl_thumbnail_url_s);

			if (image_json_p)
				{
					if (json_object_set_new (plot_json_p, "so:image", image_json_p) == 0)
						{
							success_flag = true;
						}
					else
						{
							json_decref (image_json_p);
						}
				}
		}
	else
		{
			success_flag = true;
		}


	return success_flag;
}
