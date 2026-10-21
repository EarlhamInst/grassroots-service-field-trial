/*
 * image_importer.c
 *
 *  Created on: 17 Oct 2026
 *      Author: tyrrells
 */


#include <dirent.h>


#include "image_importer.h"

#include "dfw_util.h"
#include "study.h"
#include "string_utils.h"

/*
 *
 * Get list of files
 *
 * Parse each filename to get the Study and Plot number
 *
 * Collate all of the study's images together so that we don't
 * repeatedly get the same Study
 *
 * Add images
 *
 *
 */


static bool MatchFilesOnly (const void * const data_p);

static bool MatchDirectoriesOnly (const void * const data_p);

static int SortByName (const void *v0_p, const void *v1_p);

static char **GetImageFilenamesForMatching (const char * const root_path_s);




void RunImportImages (const FieldTrialServiceData * const data_p)
{
	LinkedList *files_p = NULL;
	const char *root_path_s = data_p -> dftsd_assets_path_s;

	char **patterns_ss = GetImageFilenamesForMatching (root_path_s);

	if (patterns_ss)
		{
			char **pattern_ss = patterns_ss;

			files_p = GetMatchingFiles (patterns_ss, true, false, MatchFilesOnly);

			if (files_p)
				{
					/*
					 * Sort the list of files by name
					 */
					if (LinkedListSort (files_p, SortByName))
						{
							/*
							 * Now we need to split the filename into its
							 * separate parts. The format of the names is
							 *
							 * <study code>_<date as ddmmyy>_Fl<flight number>_<wavelength captured e.g. RGB>_Plot<plot number>.<filename extension>
							 */
							const char *delimiter_s = "_";
							StringListNode *node_p = (StringListNode *) (files_p -> ll_head_p);
							char *current_code_s = "";
							Study *current_study_p = NULL;

							enum parts
								{
									PA_CODE,
									PA_DATE,
									PA_FLIGHT_NUMBER,
									PA_WAVELENGTH,
									PA_PLOT,
									PA_NUM_PARTS
								};


							while (node_p)
								{
									LinkedList *parts_p = ParseStringToStringLinkedList (node_p -> sln_string_s, delimiter_s, false);

									if (parts_p)
										{

											if (parts_p -> ll_size == PA_NUM_PARTS)
												{
													StringListNode *parts_node_p = (StringListNode *) (parts_p -> ll_head_p);

													/*
													 * The first node contains the code, so let's check if it's changed
													 */
													if (strcmp (parts_node_p -> sln_string_s, current_code_s) != 0)
														{

															if (current_study_p)
																{
																	FreeStudy (current_study_p);
																	current_study_p = NULL;

																}
														}

 												}

											FreeLinkedList (parts_p);
										}		/* if (parts_p) */


									node_p = (StringListNode *) (node_p -> sln_node.ln_next_p);
								}		/* while (node_p) */


							ParseStringToStringLinkedList ()

						}

					FreeLinkedList (files_p);
				}		/* if (files_p) */



			while (*pattern_ss)
				{
					FreeCopiedString (*pattern_ss);
					++ pattern_ss;
				}

			FreeMemory (patterns_ss);
		}		/* if (patterns_ss) */





}




void ImportImages (const char * const code_s, const ViewFormat format, const FieldTrialServiceData * const data_p)
{
	Study *study_p = GetDFWObjectByNamedStringField (ST_CODE_S, code_s, DFTD_STUDY, GetStudyCallback, format, data_p);

}



OperationStatus ImportImage (Study *study_p, const ViewFormat format, const FieldTrialServiceData * const data_p)
{
	OperationStatus status = OS_FAILED;


	return status;
}


static char **GetImageFilenamesForMatching (const char * const root_path_s)
{
	const size_t num_patterns = 4;
	const char *patterns_ss = { "*.png", "*.jpeg", "*.jpg", "*.gif" };

	char **paths_ss = (char **) AllocMemoryArray (num_patterns + 1, (sizeof (char *)));

	if (paths_ss)
		{
			uint32 i;
			char **path_ss = paths_ss;
			char **pattern_ss = patterns_ss;
			bool success_flag = true;

			while (i < num_patterns)
				{
					char *filename_s = MakeFilename (root_path_s, *pattern_ss);

					if (filename_s)
						{
							*path_ss = filename_s;

							++ i;
							++ path_ss;
							++ pattern_ss;
						}
					else
						{
							i = num_patterns;
							success_flag = false;
						}

				}		/* while (i < num_patterns) */

			if (success_flag)
				{
					return paths_ss;
				}
			else
				{
					path_ss = paths_ss;

					while (*path_ss)
						{
							FreeCopiedString (*path_ss);
							++ path_ss;;
						}

					FreeMemory (paths_ss);
				}

		}		/* if (paths_ss) */

	return NULL;
}




static bool MatchFilesOnly (const void * const data_p)
{
	bool match_flag = false;
	const struct dirent * const entry_p = (const struct dirent * const) data_p;

	if (entry_p -> d_type == DT_UNKNOWN)
		{

		}
	else
		{
			if (entry_p -> d_type == DT_REG)
				{
					match_flag = true;
				}
		}

	return match_flag;
}



static bool MatchDirectoriesOnly (const void * const data_p)
{
	bool match_flag = false;
	const struct dirent * const entry_p = (const struct dirent * const) data_p;

	if (entry_p -> d_type == DT_UNKNOWN)
		{

		}
	else
		{
			if (entry_p -> d_type == DT_DIR)
				{
					match_flag = true;
				}
		}

	return match_flag;
}



static int SortByName (const void *v0_p, const void *v1_p)
{
	const StringListNode *sln_0_p = (const StringListNode *) v0_p;
	const StringListNode *sln_1_p = (const StringListNode *) v1_p;

	return Stricmp (sln_0_p -> sln_string_s, sln_1_p -> sln_string_s);
}





