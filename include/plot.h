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
 * plot.h
 *
 *  Created on: 23 Jul 2018
 *      Author: billy
 */

#ifndef SERVICES_FIELD_TRIALS_INCLUDE_PLOT_H_
#define SERVICES_FIELD_TRIALS_INCLUDE_PLOT_H_

#include "dfw_field_trial_service_library.h"
#include "study.h"
#include "jansson.h"


#include "linked_list.h"


#ifndef DOXYGEN_SHOULD_SKIP_THIS

#ifdef ALLOCATE_PLOT_TAGS
	#define PLOT_PREFIX DFW_FIELD_TRIAL_SERVICE_API
	#define PLOT_VAL(x)	= x
	#define PLOT_CONCAT_VAL(x,y)	= x y
#else
	#define PLOT_PREFIX extern
	#define PLOT_VAL(x)
	#define PLOT_CONCAT_VAL(x,y)
#endif

#endif 		/* #ifndef DOXYGEN_SHOULD_SKIP_THIS */


PLOT_PREFIX const char *PL_ID_S PLOT_VAL ("id");

PLOT_PREFIX const char *PL_PARENT_STUDY_S PLOT_VAL ("parent_study_id");

PLOT_PREFIX const char *PL_SOWING_DATE_S PLOT_VAL ("sowing_date");

PLOT_PREFIX const char *PL_HARVEST_DATE_S PLOT_VAL ("harvest_date");

PLOT_PREFIX const char *PL_WIDTH_S PLOT_VAL ("width");

PLOT_PREFIX const char *PL_LENGTH_S PLOT_VAL ("length");

PLOT_PREFIX const char *PL_TREATMENT_S PLOT_VAL ("treatment");

PLOT_PREFIX const char *PL_ROW_INDEX_S PLOT_VAL ("row_index");

PLOT_PREFIX const char *PL_COLUMN_INDEX_S PLOT_VAL ("column_index");

PLOT_PREFIX const char *PL_ROWS_S PLOT_VAL ("rows");

PLOT_PREFIX const char *PL_RACK_INDICES_S PLOT_VAL ("rack_indices");

PLOT_PREFIX const char *PL_COMMENT_S PLOT_VAL ("comment");

PLOT_PREFIX const char *PL_IMAGE_S PLOT_CONCAT_VAL (CONTEXT_PREFIX_SCHEMA_ORG_S, "image");

PLOT_PREFIX const char *PL_IMAGES_S PLOT_VAL ("images");

PLOT_PREFIX const char *PL_THUMBNAIL_S PLOT_CONCAT_VAL (CONTEXT_PREFIX_SCHEMA_ORG_S, "thumbnail");

PLOT_PREFIX int32 PL_UNSET_ID PLOT_VAL (INT32_MAX);

PLOT_PREFIX const char *PL_SOWING_ORDER_S PLOT_VAL ("sowing_order");

PLOT_PREFIX const char *PL_WALKING_ORDER_S PLOT_VAL ("walking_order");








typedef struct Plot
{
	bson_oid_t *pl_id_p;

	Study *pl_parent_p;

	/*
	 * Drilling
	 */

	uint32 pl_row_index;

	uint32 pl_column_index;

	struct tm *pl_sowing_date_p;

	double pl_sowing_rate;

	uint32 *pl_sowing_order_index_p;

	uint32 *pl_walking_order_index_p;

	/*
	 * Plots
	 */

	double64 *pl_width_p;

	double64 *pl_length_p;

	char *pl_treatments_s;

	char *pl_soil_type_s;

	char *pl_comment_s;


	struct tm *pl_harvest_date_p;


	/**
	 * A LinkedList of RowNodes
	 * for all of the Rows in this
	 * Plot.
	 */
	LinkedList *pl_rows_p;

	/**
	 * A LinkedList of ImageObjectNodes for any images,
	 */
	LinkedList *pl_images_p;

} Plot;


typedef struct PlotNode
{
	ListItem pn_node;

	Plot *pn_plot_p;
} PlotNode;


/*
 * forward declaration
 */
struct Row;

#ifdef __cplusplus
extern "C"
{
#endif


DFW_FIELD_TRIAL_SERVICE_LOCAL Plot *AllocatePlot (bson_oid_t *id_p, const struct tm *sowing_date_p, const struct tm *harvest_date_p, const double64 *width_p, const double64 *length_p, const uint32 row_index,
										const uint32 column_index, const char *treatments_s, const char *comment_s,
										const uint32 *walking_order_p, const uint32 *sowing_order_p, Study *parent_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL Plot *AllocateSkeletonPlot (const int32 row, const int32 column, Study *study_p, const FieldTrialServiceData *data_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL void FreePlot (Plot *plot_p);

DFW_FIELD_TRIAL_SERVICE_LOCAL PlotNode *AllocatePlotNode (Plot *plot_p);

DFW_FIELD_TRIAL_SERVICE_LOCAL void FreePlotNode (ListItem *node_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL bool AddImageToPlot (Plot *plot_p, const char * const full_image_url_s, const char * const thumbnail_s);


DFW_FIELD_TRIAL_SERVICE_LOCAL json_t *GetPlotAsJSON (Plot *plot_p, const ViewFormat format, JSONProcessor *processor_p, const FieldTrialServiceData *data_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL Plot *GetPlotFromJSON (const json_t *plot_json_p, Study *parent_study_p, const ViewFormat format, FieldTrialServiceData *data_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL bool GetPlotRows (Plot *plot_p, json_t *rows_array_p, const Study *study_p, const ViewFormat format, FieldTrialServiceData *data_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL bool SavePlot (Plot *plot_p, const FieldTrialServiceData *data_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL bool AddRowToPlot (Plot *plot_p, struct Row *row_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL struct Row *GetRowFromPlotByStudyIndex (Plot *plot_p, const uint32 by_study_index);

//DFW_FIELD_TRIAL_SERVICE_LOCAL Plot *GetPlotByIndex (const Study *study_p, const uint32 plot_index, const FieldTrialServiceData *data_p);

DFW_FIELD_TRIAL_SERVICE_LOCAL struct Row *GetRowFromPlotByStudyIndex (Plot *plot_p, const uint32 by_study_index);


DFW_FIELD_TRIAL_SERVICE_LOCAL struct Row *GetRowFromPlotByRackIndex (Plot *plot_p, const uint32 rack_index);


DFW_FIELD_TRIAL_SERVICE_LOCAL Plot *GetPlotByIdString (const char *plot_id_s, const ViewFormat format, const FieldTrialServiceData *data_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL void *GetPlotCallback (const json_t *json_p, const ViewFormat format, const FieldTrialServiceData *data_p);

#ifdef __cplusplus
}
#endif



#endif /* SERVICES_FIELD_TRIALS_INCLUDE_PLOT_H_ */
