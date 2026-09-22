/*
 * base_row.h
 *
 *  Created on: 29 Jun 2022
 *      Author: billy
 */

#ifndef SERVICES_FIELD_TRIALS_INCLUDE_BASE_ROW_H_
#define SERVICES_FIELD_TRIALS_INCLUDE_BASE_ROW_H_


#include "bson/bson.h"

#include "dfw_field_trial_service_library.h"
#include "plot.h"


/*
 * Forward declarations
 */
struct Study;


typedef enum
{
	/**
	 * A normal plot which is part of an experiment.
	 */
	RT_STANDARD,

	/**
	 * A physical plot where no measurements are taken, just there as a physical spacer
	 * which will appear in the plot-based view.
	 */
	RT_DISCARD,

	/**
	 * Used to keep the stylised view when laying out the plots and you have non-regular gaps.
	 * Grassroots won't draw these plots and will use them for gaps calculations and draw
	 * gaps instead
	 */
	RT_BLANK,


	/**
	 * A plot, usually outside the experiment, to allow the farm machinery to e.g. change
	 * their fertiliser rates, etc.
	 */
	RT_GUARD,


	/**
	 * The number of different values that a RowType can take.
	 */
	RT_NUM_VALUES
} RowType;


typedef struct Row Row;


struct Row
{
	RowType ro_type;

	bson_oid_t *ro_id_p;

	struct Study *ro_study_p;

	struct Plot *ro_plot_p;

	/**
	 * The unique index for this row within
	 * its parent study.
	 */
	uint32 ro_by_study_index;

	void (*ro_clear_fn) (Row *row_p);

	bool (*ro_add_to_json_fn) (const Row *row_p, json_t *row_json_p, const ViewFormat format, const FieldTrialServiceData *data_p);

	bool (*ro_add_from_json_fn) (Row *row_p, const json_t *row_json_p, const FieldTrialServiceData *data_p);

	bool (*ro_add_to_fd_fn) (const Row *row_p, json_t *row_fd_p, const FieldTrialServiceData *service_data_p, const char * const null_sequence_s);

};



typedef struct RowNode
{
	ListItem rn_node;

	Row *rn_row_p;
} RowNode;




#ifndef DOXYGEN_SHOULD_SKIP_THIS

#ifdef ALLOCATE_BASE_ROW_TAGS
	#define BASE_ROW_PREFIX DFW_FIELD_TRIAL_SERVICE_API
	#define BASE_ROW_VAL(x)	= x
	#define BASE_ROW_CONCAT_VAL(x,y)	= x y
#else
	#define BASE_ROW_PREFIX extern
	#define BASE_ROW_VAL(x)
	#define BASE_ROW_CONCAT_VAL(x,y)
#endif

#endif 		/* #ifndef DOXYGEN_SHOULD_SKIP_THIS */




BASE_ROW_PREFIX const char *RO_ID_S BASE_ROW_VAL ("id");

BASE_ROW_PREFIX const char *RO_STUDY_INDEX_S BASE_ROW_VAL ("study_index");

BASE_ROW_PREFIX const char *RO_PLOT_ID_S BASE_ROW_VAL ("plot_id");

BASE_ROW_PREFIX const char *RO_STUDY_ID_S BASE_ROW_VAL ("study_id");

BASE_ROW_PREFIX const char *RO_DISCARD_S BASE_ROW_VAL ("discard");


BASE_ROW_PREFIX const char *RO_BLANK_S BASE_ROW_VAL ("blank");

BASE_ROW_PREFIX const char *RO_GUARD_S BASE_ROW_VAL ("guard");


BASE_ROW_PREFIX const char *RO_ROW_TYPE_S BASE_ROW_VAL ("row_type");




#ifdef __cplusplus
extern "C"
{
#endif





DFW_FIELD_TRIAL_SERVICE_LOCAL Row *AllocateDiscardRow (bson_oid_t *id_p, const uint32 study_index, Plot *parent_plot_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL Row *AllocateBaseRow (bson_oid_t *id_p, const uint32 study_index, Plot *parent_plot_p, RowType rt,
																												void (*clear_fn) (Row *row_p),
																												bool (*add_to_json_fn) (const Row *row_p, json_t *row_json_p, const ViewFormat format, const FieldTrialServiceData *data_p),
																												bool (*add_from_json_fn) (Row *row_p, const json_t *row_json_p, const FieldTrialServiceData *data_p),
																												bool (*add_to_fd_fn) (const Row *row_p, json_t *row_fd_p, const FieldTrialServiceData *service_data_p, const char * const null_sequence_s));


DFW_FIELD_TRIAL_SERVICE_LOCAL  bool InitRow (Row *row_p, bson_oid_t *id_p, const uint32 study_index, Plot *parent_plot_p, RowType rt,
																								 void (*clear_fn) (Row *row_p),
																								 bool (*add_to_json_fn) (const Row *row_p, json_t *row_json_p, const ViewFormat format, const FieldTrialServiceData *data_p),
																								 bool (*add_from_json_fn) (Row *row_p, const json_t *row_json_p, const FieldTrialServiceData *data_p),
																								 bool (*add_to_fd_fn) (const Row *row_p, json_t *row_fd_p, const FieldTrialServiceData *service_data_p, const char * const null_sequence_s));


DFW_FIELD_TRIAL_SERVICE_LOCAL void ClearRow (Row *row_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL void FreeRow (Row *row_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL RowNode *AllocateRowNode (Row *row_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL void FreeRowNode (ListItem *node_p);



DFW_FIELD_TRIAL_SERVICE_LOCAL void SetRowType (Row *row_p, RowType rt);


DFW_FIELD_TRIAL_SERVICE_LOCAL RowType GetRowType (const Row *row_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL bool AddRowToJSON (const Row *row_p, json_t *row_json_p, const ViewFormat format, const FieldTrialServiceData *data_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL Row *GetRowFromJSON (const json_t *json_p, Plot *plot_p, const Study *study_p, const ViewFormat format, FieldTrialServiceData *data_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL bool PopulateRowFromJSON (Row *row_p, Plot *plot_p, const json_t *row_json_p, const ViewFormat format, FieldTrialServiceData *data_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL json_t *GetRowAsJSON (const Row *row_p, const ViewFormat format, JSONProcessor *processor_p, const FieldTrialServiceData *data_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL bool SetRowTypeFromJSON (RowType *rt_p, const json_t *row_json_p);


DFW_FIELD_TRIAL_SERVICE_LOCAL const char *GetRowTypeAsString (const RowType rt);


DFW_FIELD_TRIAL_SERVICE_LOCAL bool SetRowTypeFromString (RowType *rt_p, const char *value_s);


DFW_FIELD_TRIAL_SERVICE_LOCAL void SetRowCallbackFunctions (Row *row_p,
																														void (*clear_fn) (Row *row_p),
																														bool (*add_to_json_fn) (const Row *row_p, json_t *row_json_p, const ViewFormat format, const FieldTrialServiceData *data_p),
																														bool (*add_from_json_fn) (Row *row_p, const json_t *row_json_p, const FieldTrialServiceData *data_p),
																														bool (*add_to_fd_fn) (const Row *row_p, json_t *row_fd_p, const FieldTrialServiceData *service_data_p, const char * const null_sequence_s));




#ifdef __cplusplus
}
#endif

#endif /* SERVICES_FIELD_TRIALS_INCLUDE_BASE_ROW_H_ */
