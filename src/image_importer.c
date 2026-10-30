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
#include "study_jobs.h"
#include "row_jobs.h"
#include "mongodb_tool.h"
#include "math_utils.h"
#include "filesystem_utils.h"



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


static bool CopyImageFile (const char *full_image_filename_s, const char *current_study_id_s, const uint32 study_index, char **image_url_ss, const FieldTrialServiceData * const data_p);


static bool GetStudyIndex (const char * const plot_s, uint32 *index_p);

static bool IsRotatedImage (const char * const full_image_filename_s);




OperationStatus RunImportImages (const char * const import_path_s, const FieldTrialServiceData * const data_p)
{
	OperationStatus status = OS_FAILED_TO_START;
	LinkedList *files_p = NULL;

	char **patterns_ss = GetImageFilenamesForMatching (import_path_s);

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
							char current_study_id_s [MONGO_OID_STRING_BUFFER_SIZE] = {};
							uint32 num_successes = 0;

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
									const char * const full_image_filename_s = node_p -> sln_string_s;
									LinkedList *parts_p = ParseStringToStringLinkedList (full_image_filename_s, delimiter_s, false);

									if (parts_p)
										{
											if (parts_p -> ll_size == PA_NUM_PARTS)
												{
													StringListNode *parts_node_p = (StringListNode *) (parts_p -> ll_head_p);
													char *code_s = GetFilenameOnly (parts_node_p -> sln_string_s);

													if (code_s)
														{
															/*
															 * The first node contains the code, so let's check if it's changed
															 */
															if (strcmp (code_s, current_code_s) != 0)
																{

																	if (current_study_p)
																		{
																			FreeStudy (current_study_p);
																			current_study_p = NULL;

																		}

																	current_study_p = GetStudyByCode (code_s, data_p);

																	if (current_study_p)
																		{
																			PopulateBSONOidIntoString (current_study_p -> st_id_p, current_study_id_s);
																		}
																	else
																		{

																		}

																}		/* if (strcmp (parts_node_p -> sln_string_s, current_code_s) != 0) */

															/* Now find the matching plot and add the image to its list */
															if (current_study_p)
																{
																	/* Let's get the plot */
																	const char *date_s;
																	const char *flight_s;
																	const char *wavelength_s;
																	const char *plot_s;
																	uint32 study_index = 0;
																	Row *row_p = NULL;

																	parts_node_p = (StringListNode *) (parts_node_p -> sln_node.ln_next_p);
																	date_s = parts_node_p -> sln_string_s;

																	parts_node_p = (StringListNode *) (parts_node_p -> sln_node.ln_next_p);
																	flight_s = parts_node_p -> sln_string_s;

																	parts_node_p = (StringListNode *) (parts_node_p -> sln_node.ln_next_p);
																	wavelength_s = parts_node_p -> sln_string_s;

																	parts_node_p = (StringListNode *) (parts_node_p -> sln_node.ln_next_p);
																	plot_s = parts_node_p -> sln_string_s;


																	if (GetStudyIndex (plot_s, &study_index))
																		{
																			char *image_url_s = NULL;

																			if (CopyImageFile (full_image_filename_s, current_study_id_s, study_index, &image_url_s, data_p))
																				{
																					/*
																					 * Get the study index from plot_s
																					 */
																					row_p = GetRowByStudyIndex (study_index, current_study_p, VF_STORAGE, data_p);

																					if (row_p)
																						{
																							Plot *plot_p = row_p -> ro_plot_p;
																							const char *thumbnail_url_s = NULL;
																							ImageObject *image_p = AllocateImageObject (image_url_s, thumbnail_url_s);

																							if (image_p)
																								{
																									if (AddImageToPlot (plot_p, image_p))
																										{
																											if (SavePlot (plot_p, data_p))
																												{
																													++ num_successes;
																												}
																											else
																												{

																												}

																										}
																									else
																										{
																											PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "AddImageToPlot () failed for \"%s\", \"%s\"", image_url_s,
																																	 thumbnail_url_s ? thumbnail_url_s : "");

																											FreeImageObject (image_p);
																										}

																								}		/* if (image_p) */
																							else
																								{
																									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "AllocateImageObject () failed for \"%s\", \"%s\"", image_url_s,
																															 thumbnail_url_s ? thumbnail_url_s : "");

																								}


																						}		/* if (row_p) */
																					else
																						{
																							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "GetRowByStudyIndex () failed for plot index " UINT32_FMT " from study \"%s\"", study_index, current_study_p -> st_name_s);

																						}


																					FreeCopiedString (image_url_s);
																				}		/* if (CopyImageFile (full_image_filename_s, current_study_id_s, &image_url_s, data_p)) */


																		}		/* if (GetStudyIndex (plot_s, &study_index)) */
																	else
																		{
																			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to get study_index from \"%s\"", plot_s);
																		}


																}		/* if (current_study_p) */

															FreeCopiedString (code_s);
														}		/* if (code_s) */

 												}		/* if (parts_p -> ll_size == PA_NUM_PARTS) */

											FreeLinkedList (parts_p);
										}		/* if (parts_p) */

									node_p = (StringListNode *) (node_p -> sln_node.ln_next_p);
								}		/* while (node_p) */

							if (current_study_p)
								{
									FreeStudy (current_study_p);
								}

							if (num_successes == files_p -> ll_size)
								{
									status = OS_SUCCEEDED;
								}
							else if (num_successes > 0)
								{
									status = OS_PARTIALLY_SUCCEEDED;
								}

						}		/* if (LinkedListSort (files_p, SortByName)) */

					FreeLinkedList (files_p);
				}		/* if (files_p) */



			while (*pattern_ss)
				{
					FreeCopiedString (*pattern_ss);
					++ pattern_ss;
				}

			FreeMemory (patterns_ss);
		}		/* if (patterns_ss) */


	return status;
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



/**
 * Copy the image file into the appropriate directory and calculate the url for the copied file.
 *
 * @param full_image_filename_s The path to the image file that will be used.
 * @param current_study_id_s The Id for the current sStudy as a string.
 * @param study_index The study index of this plot within its Study.
 * @param image_url_ss A pointer to the string where the resultant image url will be set upon success.
 * @param data_p The configured FieldTrialServiceData.
 * @return <code>true</code> if the file was copied successfully and the image url set, <code>false</code> otherwise.
 */
static bool CopyImageFile (const char *full_image_filename_s, const char *current_study_id_s, const uint32 study_index, char **image_url_ss, const FieldTrialServiceData * const data_p)
{
	bool success_flag = false;


	char *study_index_s = ConvertUnsignedIntegerToString (study_index);

	if (study_index_s)
		{
			char sep_s [2];
			const char c = GetFileSeparatorChar ();
			const size_t l = strlen (data_p -> dftsd_assets_path_s);
			char *study_assets_path_s;

			*sep_s = c;
			* (sep_s + 1) = '\0';


			if (IsRotatedImage (full_image_filename_s))
				{
					const char * const rotated_s = "rotated";
					char *rotated_dir_s = MakeFilename (study_assets_path_s, rotated_s);

					if (rotated_dir_s)
						{

							if (EnsureDirectoryExists (study_assets_path_s))
								{

								}

							FreeCopiedString (rotated_dir_s);
						}

				}
			else
				{
					if ((* ((data_p -> dftsd_assets_path_s) + l - 1)) == c)
						{
							study_assets_path_s = ConcatenateVarargsStrings (data_p -> dftsd_assets_path_s, current_study_id_s, sep_s, "plot_", study_index_s, NULL);
						}
					else
						{
							study_assets_path_s = ConcatenateVarargsStrings (data_p -> dftsd_assets_path_s, sep_s, current_study_id_s, sep_s, "plot_", study_index_s, NULL);
						}
				}

			if (study_assets_path_s)
				{
					if (EnsureDirectoryExists (study_assets_path_s))
						{
							/* Make sure that the directory for the rotated images exists */


							char *local_image_filename_s = GetFilenameOnly (full_image_filename_s);

							if (local_image_filename_s)
								{
									char *dest_filename_s = ConcatenateVarargsStrings (study_assets_path_s, sep_s, local_image_filename_s, NULL);

									if (dest_filename_s)
										{
											if (CopyToNewFile (full_image_filename_s, dest_filename_s, NULL))
												{
													const char * const filename_from_root_s = dest_filename_s + strlen (data_p -> dftsd_assets_path_s);

													char *image_url_s = MakeFilename (data_p -> dftsd_fd_url_s, filename_from_root_s);

													if (image_url_s)
														{
															*image_url_ss = image_url_s;
															success_flag = true;
														}		/* if (image_url_s) */
													else
														{
															PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "MakeFilename () failed for \"%s\" and \"%s\"", data_p -> dftsd_fd_url_s, filename_from_root_s);
														}

												}		/* if (CopyToNewFile (full_image_filename_s, dest_filename_s)) */
											else
												{
													PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "CopyToNewFile () failed for \"%s\" and \"%s\"", full_image_filename_s, dest_filename_s);
												}

											FreeCopiedString (dest_filename_s);
										}		/* if (dest_filename_s) */
									else
										{
											PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "MakeFilename () failed for \"%s\" and \"%s\"", study_assets_path_s, local_image_filename_s);
										}

									FreeCopiedString (local_image_filename_s);
								}		/* if (local_image_filename_s) */
							else
								{
									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "GetFilenameOnly () failed for \"%s\"", full_image_filename_s);
								}

						}		/* if (EnsureDirectoryExists (study_assets_path_s)) */
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "EnsureDirectoryExists () failed for \"%s\"", study_assets_path_s);
						}

					FreeCopiedString (study_assets_path_s);
				}		/* if (study_assets_path_s) */
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "MakeFilename () failed for \"%s\" and \"%s\"", data_p -> dftsd_assets_path_s, current_study_id_s);
				}

			FreeCopiedString (study_index_s);
		}		/* if (study_index_s) */
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "ConvertUnsignedIntegerToString () failed for " UINT32_FMT, study_index);
		}

	return success_flag;
}


/*
 * Each Plot will have a directory like <study id>/Plot_<study index>. Inside this, all of the
 * images at the top level will be the those in their natural orientation. There will also be 3
 * subdirectories:
 *
 * metadata: where all of the image metadata goes
 * rotated: where all of the rotated images in portrait/landscape view are stored
 *
 * Both the top-level and rotated directories will also have a thumbnails directory where their
 * respective thumbnails will be stored.
 *
 * So we have
 *
 * PLOT directory
 *   |
 *   -- image files
 *   |
 *   -- metadata directory
 *   |
 *   -- thumbnails directory
 *   |
 *   -- rotated directory
 *     |
 *     -- rotated thumbnails directory
 */
static bool CreatePlotImageDirectories ()
{

}


static bool GetStudyIndex (const char * const plot_s, uint32 *index_p)
{
	bool success_flag = false;
	const char *c_p = plot_s;
	const char *start_p = NULL;

	/* Get the start of the plot index */
	while ((*c_p != '\0') && (start_p == NULL))
		{
			if (isdigit (*c_p))
				{
					start_p = c_p;
				}
			else
				{
					++ c_p;
				}
		}

	if (start_p)
		{
			char *end_p = NULL;
			unsigned long i;

			errno = 0;

			i = strtoul (start_p, &end_p, 10);

			if (errno == 0)
				{
					*index_p = i;
					success_flag = true;
				}

		}


	return success_flag;
}


static char **GetImageFilenamesForMatching (const char * const root_path_s)
{
	const size_t num_patterns = 4;
	const char *patterns_ss [] = { "*.png", "*.jpeg", "*.jpg", "*.gif" };

	char **paths_ss = (char **) AllocMemoryArray (num_patterns + 1, (sizeof (char *)));

	if (paths_ss)
		{
			uint32 i;
			char **path_ss = paths_ss;
			const char **pattern_ss = patterns_ss;
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
	const StringListNode **sln_0_pp = (const StringListNode **) v0_p;
	const StringListNode **sln_1_pp = (const StringListNode **) v1_p;

	return Stricmp ((*sln_0_pp) -> sln_string_s, (*sln_1_pp) -> sln_string_s);
}



static bool IsRotatedImage (const char * const full_image_filename_s)
{
	bool rotated_flag = false;

	return rotated_flag;
}


