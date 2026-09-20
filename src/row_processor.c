/*
 * row_processor.c
 *
 *  Created on: 5 Dec 2019
 *      Author: billy
 */

#include "row_processor.h"
#include "row.h"
#include "streams.h"
#include "standard_row.h"



static json_t *HighlightPlotsContainingMaterial (struct JSONProcessor *processor_p, struct Row *plot_p, ViewFormat format, const FieldTrialServiceData *service_data_p);


JSONProcessor *AllocateRowProcessor (struct Material *material_p)
{
	RowProcessor *processor_p = (RowProcessor *) AllocMemory (sizeof (RowProcessor));

	if (processor_p)
		{
			InitialiseJSONProcessor (& (processor_p -> rp_base_processor), NULL, HighlightPlotsContainingMaterial, NULL);
			processor_p -> rp_material_p = material_p;

			return (& (processor_p -> rp_base_processor));
		}

	return NULL;
}


static json_t *HighlightPlotsContainingMaterial (struct JSONProcessor *processor_p, Row *row_p, ViewFormat format, const FieldTrialServiceData *service_data_p)
{
	RowProcessor *row_processor_p = (RowProcessor *) processor_p;
	json_t *row_json_p = GetRowAsJSON (row_p, format, NULL, service_data_p);

	if (row_json_p)
		{
			if (row_p -> ro_type == RT_STANDARD)
				{
					StandardRow *standard_row_p = (StandardRow *) row_p;

					if ((row_processor_p -> rp_material_p) && (standard_row_p -> sr_planted_material_p))
						{
							if (bson_oid_compare (row_processor_p -> rp_material_p -> ma_id_p, standard_row_p -> sr_planted_material_p -> ma_id_p) == 0)
								{
									if (!SetJSONBoolean (row_json_p, DFT_SELECTED_S, true))
										{
											PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, row_json_p, "Failed to add \"%s\": true for Material \"%s\"", DFT_SELECTED_S, row_processor_p -> rp_material_p -> ma_accession_s);
										}
								}
						}
				}



		}

	return row_json_p;
}
