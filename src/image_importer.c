/*
 * image_importer.c
 *
 *  Created on: 17 Oct 2026
 *      Author: tyrrells
 */


#include <dirent.h>

#include "libexif/exif-data.h"

/*
 * stb is used to create the thumbnails. Its functions are made static so that
 * they don't clash with any other copies of stb loaded by the server.
 */
#if defined (__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_JPEG
#define STBI_ONLY_PNG
#define STBI_ONLY_GIF
#include "stb/stb_image.h"

#define STB_IMAGE_WRITE_STATIC
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"

#if defined (__GNUC__)
#pragma GCC diagnostic pop
#endif

#include "image_importer.h"

#include "dfw_util.h"
#include "study.h"
#include "string_utils.h"
#include "study_jobs.h"
#include "row_jobs.h"
#include "mongodb_tool.h"
#include "math_utils.h"
#include "filesystem_utils.h"
#include "memory_allocations.h"



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


static bool CopyImageFile (const char *full_image_filename_s, const char *current_study_id_s, const uint32 study_index, char **image_url_ss, char **thumbnail_url_ss, const FieldTrialServiceData * const data_p);

static char *GetAssetUrl (const char * const asset_filename_s, const FieldTrialServiceData * const data_p);

static char *CreateThumbnailForImage (const char * const image_filename_s, const char * const image_dir_s, const char * const local_image_filename_s, const FieldTrialServiceData * const data_p);

static bool CreateThumbnail (const char * const image_filename_s, const char * const thumbnail_filename_s, const int max_width, const int max_height);

static int GetExifOrientation (const char * const image_filename_s);

static unsigned char *ShrinkImage (const unsigned char * const src_p, const int src_width, const int src_height, const int num_channels, const int dest_width, const int dest_height);

static unsigned char *OrientImage (const unsigned char * const src_p, const int src_width, const int src_height, const int num_channels, const int orientation);

static bool HasFileExtension (const char * const filename_s, const char * const extension_s);


static bool GetStudyIndex (const char * const plot_s, uint32 *index_p);

static bool IsRotatedImage (const char * const full_image_filename_s);

static bool CreatePlotImageDirectories (const char * const plot_dir_s);

static bool EnsureSubdirectoryExists (const char * const parent_dir_s, const char * const subdir_s, char **full_subdir_ss);


static const char * const S_METADATA_DIR_S = "metadata";

static const char * const S_THUMBNAILS_DIR_S = "thumbnails";

static const char * const S_ROTATED_DIR_S = "rotated";

static const int S_THUMBNAIL_MAX_WIDTH = 640;

static const int S_THUMBNAIL_MAX_HEIGHT = 256;




OperationStatus RunImportImages (const char * const import_path_s, const FieldTrialServiceData * const data_p)
{
	OperationStatus status = OS_FAILED_TO_START;
	LinkedList *files_p = NULL;
	char **patterns_ss = NULL;

	patterns_ss = GetImageFilenamesForMatching (import_path_s);

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
																			char *thumbnail_url_s = NULL;

																			if (CopyImageFile (full_image_filename_s, current_study_id_s, study_index, &image_url_s, &thumbnail_url_s, data_p))
																				{
																					/*
																					 * Get the study index from plot_s
																					 */
																					row_p = GetRowByStudyIndex (study_index, current_study_p, VF_STORAGE, data_p);

																					if (row_p)
																						{
																							Plot *plot_p = row_p -> ro_plot_p;
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

																					if (thumbnail_url_s)
																						{
																							FreeCopiedString (thumbnail_url_s);
																						}

																				}		/* if (CopyImageFile (full_image_filename_s, current_study_id_s, &image_url_s, &thumbnail_url_s, data_p)) */


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
 * Copy the image file into the appropriate directory, create its thumbnail and calculate the urls
 * for both of them.
 *
 * @param full_image_filename_s The path to the image file that will be used.
 * @param current_study_id_s The Id for the current sStudy as a string.
 * @param study_index The study index of this plot within its Study.
 * @param image_url_ss A pointer to the string where the resultant image url will be set upon success.
 * @param thumbnail_url_ss A pointer to the string where the resultant thumbnail url will be set. If the
 * thumbnail could not be created, this will be set to <code>NULL</code>.
 * @param data_p The configured FieldTrialServiceData.
 * @return <code>true</code> if the file was copied successfully and the image url set, <code>false</code> otherwise.
 */
static bool CopyImageFile (const char *full_image_filename_s, const char *current_study_id_s, const uint32 study_index, char **image_url_ss, char **thumbnail_url_ss, const FieldTrialServiceData * const data_p)
{
	bool success_flag = false;


	char *study_index_s = ConvertUnsignedIntegerToString (study_index);

	*thumbnail_url_ss = NULL;

	if (study_index_s)
		{
			char sep_s [2];
			const char c = GetFileSeparatorChar ();
			const size_t l = strlen (data_p -> dftsd_assets_path_s);
			char *study_assets_path_s;

			*sep_s = c;
			* (sep_s + 1) = '\0';


			if ((* ((data_p -> dftsd_assets_path_s) + l - 1)) == c)
				{
					study_assets_path_s = ConcatenateVarargsStrings (data_p -> dftsd_assets_path_s, current_study_id_s, sep_s, "plot_", study_index_s, NULL);
				}
			else
				{
					study_assets_path_s = ConcatenateVarargsStrings (data_p -> dftsd_assets_path_s, sep_s, current_study_id_s, sep_s, "plot_", study_index_s, NULL);
				}


			if (study_assets_path_s)
				{
					if (CreatePlotImageDirectories (study_assets_path_s))
						{
							/* Rotated images go into their own subdirectory */
							char *dest_dir_s = IsRotatedImage (full_image_filename_s) ? MakeFilename (study_assets_path_s, S_ROTATED_DIR_S) : study_assets_path_s;

							if (dest_dir_s)
								{
									char *local_image_filename_s = GetFilenameOnly (full_image_filename_s);

									if (local_image_filename_s)
										{
											char *dest_filename_s = MakeFilename (dest_dir_s, local_image_filename_s);

											if (dest_filename_s)
												{
													if (CopyToNewFile (full_image_filename_s, dest_filename_s, NULL))
														{
															char *image_url_s = GetAssetUrl (dest_filename_s, data_p);

															if (image_url_s)
																{
																	*image_url_ss = image_url_s;
																	success_flag = true;

																	/*
																	 * A missing thumbnail isn't fatal as the image itself is still
																	 * available.
																	 */
																	*thumbnail_url_ss = CreateThumbnailForImage (dest_filename_s, dest_dir_s, local_image_filename_s, data_p);
																}		/* if (image_url_s) */

														}		/* if (CopyToNewFile (full_image_filename_s, dest_filename_s)) */
													else
														{
															PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "CopyToNewFile () failed for \"%s\" and \"%s\"", full_image_filename_s, dest_filename_s);
														}

													FreeCopiedString (dest_filename_s);
												}		/* if (dest_filename_s) */
											else
												{
													PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "MakeFilename () failed for \"%s\" and \"%s\"", dest_dir_s, local_image_filename_s);
												}

											FreeCopiedString (local_image_filename_s);
										}		/* if (local_image_filename_s) */
									else
										{
											PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "GetFilenameOnly () failed for \"%s\"", full_image_filename_s);
										}

									if (dest_dir_s != study_assets_path_s)
										{
											FreeCopiedString (dest_dir_s);
										}

								}		/* if (dest_dir_s) */
							else
								{
									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "MakeFilename () failed for \"%s\" and \"%s\"", study_assets_path_s, S_ROTATED_DIR_S);
								}

						}		/* if (CreatePlotImageDirectories (study_assets_path_s)) */
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "CreatePlotImageDirectories () failed for \"%s\"", study_assets_path_s);
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


/**
 * Get the url for a file that is stored within the configured assets directory.
 *
 * @param asset_filename_s The full path to the file. This must be within the assets directory.
 * @param data_p The configured FieldTrialServiceData.
 * @return The url which the caller must free with FreeCopiedString () or <code>NULL</code> upon error.
 */
static char *GetAssetUrl (const char * const asset_filename_s, const FieldTrialServiceData * const data_p)
{
	const char * const filename_from_root_s = asset_filename_s + strlen (data_p -> dftsd_assets_path_s);
	char *url_s = MakeFilename (data_p -> dftsd_fd_url_s, filename_from_root_s);

	if (!url_s)
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "MakeFilename () failed for \"%s\" and \"%s\"", data_p -> dftsd_fd_url_s, filename_from_root_s);
		}

	return url_s;
}


/**
 * Create a thumbnail for an image that has been copied into a plot's directory.
 *
 * @param image_filename_s The full path to the copied image.
 * @param image_dir_s The directory containing the copied image. The thumbnail will be
 * stored in its thumbnails subdirectory.
 * @param local_image_filename_s The filename, without any path, of the image. The thumbnail
 * will use the same filename.
 * @param data_p The configured FieldTrialServiceData.
 * @return The url of the thumbnail which the caller must free with FreeCopiedString ()
 * or <code>NULL</code> upon error.
 */
static char *CreateThumbnailForImage (const char * const image_filename_s, const char * const image_dir_s, const char * const local_image_filename_s, const FieldTrialServiceData * const data_p)
{
	char *thumbnail_url_s = NULL;
	char *thumbnails_dir_s = MakeFilename (image_dir_s, S_THUMBNAILS_DIR_S);

	if (thumbnails_dir_s)
		{
			/*
			 * stb can only write jpeg and png files so any other formats,
			 * e.g. gif, will have png thumbnails.
			 */
			char *thumbnail_local_filename_s = NULL;

			if (HasFileExtension (local_image_filename_s, "jpg") || HasFileExtension (local_image_filename_s, "jpeg") || HasFileExtension (local_image_filename_s, "png"))
				{
					thumbnail_local_filename_s = EasyCopyToNewString (local_image_filename_s);
				}
			else
				{
					thumbnail_local_filename_s = ConcatenateStrings (local_image_filename_s, ".png");
				}

			if (thumbnail_local_filename_s)
				{
					char *thumbnail_filename_s = MakeFilename (thumbnails_dir_s, thumbnail_local_filename_s);

					if (thumbnail_filename_s)
						{
							if (CreateThumbnail (image_filename_s, thumbnail_filename_s, data_p -> , S_THUMBNAIL_MAX_HEIGHT))
								{
									thumbnail_url_s = GetAssetUrl (thumbnail_filename_s, data_p);
								}

							FreeCopiedString (thumbnail_filename_s);
						}		/* if (thumbnail_filename_s) */
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "MakeFilename () failed for \"%s\" and \"%s\"", thumbnails_dir_s, thumbnail_local_filename_s);
						}

					FreeCopiedString (thumbnail_local_filename_s);
				}		/* if (thumbnail_local_filename_s) */
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to create thumbnail filename for \"%s\"", local_image_filename_s);
				}

			FreeCopiedString (thumbnails_dir_s);
		}		/* if (thumbnails_dir_s) */
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "MakeFilename () failed for \"%s\" and \"%s\"", image_dir_s, S_THUMBNAILS_DIR_S);
		}

	return thumbnail_url_s;
}


/**
 * Create a thumbnail of an image which will fit within the given dimensions whilst
 * keeping the original aspect ratio. Any EXIF orientation will be applied so that the
 * thumbnail is the right way up.
 *
 * @param image_filename_s The image to create the thumbnail for.
 * @param thumbnail_filename_s The filename to save the thumbnail to. If it has a jpg or jpeg
 * extension, it will be saved as a jpeg, otherwise it will be saved as a png.
 * @param max_width The maximum width of the thumbnail.
 * @param max_height The maximum height of the thumbnail.
 * @return <code>true</code> if the thumbnail was created successfully, <code>false</code> otherwise.
 */
static bool CreateThumbnail (const char * const image_filename_s, const char * const thumbnail_filename_s, const int max_width, const int max_height)
{
	bool success_flag = false;
	int width;
	int height;
	int num_file_channels;

	if (stbi_info (image_filename_s, &width, &height, &num_file_channels))
		{
			/* Keep any alpha channel and convert greyscale to RGB */
			const int num_channels = ((num_file_channels == 2) || (num_file_channels == 4)) ? 4 : 3;
			unsigned char *image_p = stbi_load (image_filename_s, &width, &height, &num_file_channels, num_channels);

			if (image_p)
				{
					const int orientation = GetExifOrientation (image_filename_s);

					/* Orientations 5 - 8 swap the width and height */
					const bool swap_flag = (orientation >= 5);
					const double oriented_width = swap_flag ? height : width;
					const double oriented_height = swap_flag ? width : height;
					double scale = 1.0;
					int thumbnail_width;
					int thumbnail_height;
					unsigned char *shrunk_image_p = NULL;

					if (oriented_width > max_width)
						{
							scale = max_width / oriented_width;
						}

					if (oriented_height * scale > max_height)
						{
							scale = max_height / oriented_height;
						}

					thumbnail_width = (int) (width * scale + 0.5);
					thumbnail_height = (int) (height * scale + 0.5);

					if (thumbnail_width < 1)
						{
							thumbnail_width = 1;
						}

					if (thumbnail_height < 1)
						{
							thumbnail_height = 1;
						}

					shrunk_image_p = ShrinkImage (image_p, width, height, num_channels, thumbnail_width, thumbnail_height);

					if (shrunk_image_p)
						{
							unsigned char *thumbnail_p = OrientImage (shrunk_image_p, thumbnail_width, thumbnail_height, num_channels, orientation);

							if (thumbnail_p)
								{
									int res;

									if (swap_flag)
										{
											const int temp = thumbnail_width;
											thumbnail_width = thumbnail_height;
											thumbnail_height = temp;
										}

									if (HasFileExtension (thumbnail_filename_s, "jpg") || HasFileExtension (thumbnail_filename_s, "jpeg"))
										{
											res = stbi_write_jpg (thumbnail_filename_s, thumbnail_width, thumbnail_height, num_channels, thumbnail_p, 90);
										}
									else
										{
											res = stbi_write_png (thumbnail_filename_s, thumbnail_width, thumbnail_height, num_channels, thumbnail_p, thumbnail_width * num_channels);
										}

									if (res != 0)
										{
											success_flag = true;
										}
									else
										{
											PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to write thumbnail \"%s\"", thumbnail_filename_s);
										}

									if (thumbnail_p != shrunk_image_p)
										{
											FreeMemory (thumbnail_p);
										}
								}		/* if (thumbnail_p) */
							else
								{
									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to orient thumbnail for \"%s\"", image_filename_s);
								}

							FreeMemory (shrunk_image_p);
						}		/* if (shrunk_image_p) */
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to shrink \"%s\" to " INT32_FMT " x " INT32_FMT, image_filename_s, thumbnail_width, thumbnail_height);
						}

					stbi_image_free (image_p);
				}		/* if (image_p) */
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "stbi_load () failed for \"%s\": %s", image_filename_s, stbi_failure_reason ());
				}

		}		/* if (stbi_info (image_filename_s, &width, &height, &num_file_channels)) */
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "stbi_info () failed for \"%s\": %s", image_filename_s, stbi_failure_reason ());
		}

	return success_flag;
}


/**
 * Get the EXIF orientation of an image.
 *
 * @param image_filename_s The image to check.
 * @return The orientation from 1 to 8 as defined by the EXIF specification. If the image
 * has no valid orientation, 1 will be returned meaning that no transformation is needed.
 */
static int GetExifOrientation (const char * const image_filename_s)
{
	int orientation = 1;
	ExifData *exif_p = exif_data_new_from_file (image_filename_s);

	if (exif_p)
		{
			ExifEntry *entry_p = exif_data_get_entry (exif_p, EXIF_TAG_ORIENTATION);

			if ((entry_p) && (entry_p -> format == EXIF_FORMAT_SHORT))
				{
					const int value = exif_get_short (entry_p -> data, exif_data_get_byte_order (exif_p));

					if ((value >= 1) && (value <= 8))
						{
							orientation = value;
						}
				}

			exif_data_unref (exif_p);
		}

	return orientation;
}


/**
 * Shrink an image by averaging each block of source pixels that maps onto
 * a destination pixel.
 *
 * @param src_p The source pixels.
 * @param src_width The width of the source image.
 * @param src_height The height of the source image.
 * @param num_channels The number of bytes per pixel in both images.
 * @param dest_width The width of the shrunk image. This must not be greater than src_width.
 * @param dest_height The height of the shrunk image. This must not be greater than src_height.
 * @return The shrunk image which the caller must free with FreeMemory () or <code>NULL</code> upon error.
 */
static unsigned char *ShrinkImage (const unsigned char * const src_p, const int src_width, const int src_height, const int num_channels, const int dest_width, const int dest_height)
{
	unsigned char *dest_p = (unsigned char *) AllocMemory ((size_t) dest_width * (size_t) dest_height * (size_t) num_channels);

	if (dest_p)
		{
			unsigned char *dest_pixel_p = dest_p;
			int y;

			for (y = 0; y < dest_height; ++ y)
				{
					const int y0 = (int) (((int64) y * src_height) / dest_height);
					int y1 = (int) (((int64) (y + 1) * src_height) / dest_height);
					int x;

					if (y1 <= y0)
						{
							y1 = y0 + 1;
						}

					for (x = 0; x < dest_width; ++ x)
						{
							const int x0 = (int) (((int64) x * src_width) / dest_width);
							int x1 = (int) (((int64) (x + 1) * src_width) / dest_width);
							uint32 count;
							int c;

							if (x1 <= x0)
								{
									x1 = x0 + 1;
								}

							count = (uint32) ((x1 - x0) * (y1 - y0));

							for (c = 0; c < num_channels; ++ c)
								{
									uint32 total = 0;
									int i;

									for (i = y0; i < y1; ++ i)
										{
											const unsigned char *src_pixel_p = src_p + (((size_t) i * src_width) + x0) * num_channels + c;
											int j;

											for (j = x0; j < x1; ++ j, src_pixel_p += num_channels)
												{
													total += *src_pixel_p;
												}
										}

									*dest_pixel_p = (unsigned char) ((total + (count / 2)) / count);
									++ dest_pixel_p;
								}
						}
				}
		}

	return dest_p;
}


/**
 * Apply an EXIF orientation to an image so that it is the right way up.
 *
 * @param src_p The source pixels.
 * @param src_width The width of the source image.
 * @param src_height The height of the source image.
 * @param num_channels The number of bytes per pixel.
 * @param orientation The EXIF orientation from 1 to 8. For values of 5 to 8, the
 * width and height of the resultant image are swapped.
 * @return If orientation is 1, src_p is returned. Otherwise it is a new image which the caller
 * must free with FreeMemory () or <code>NULL</code> upon error.
 */
static unsigned char *OrientImage (const unsigned char * const src_p, const int src_width, const int src_height, const int num_channels, const int orientation)
{
	unsigned char *dest_p = NULL;
	const bool swap_flag = (orientation >= 5);
	const int dest_width = swap_flag ? src_height : src_width;
	const int dest_height = swap_flag ? src_width : src_height;

	if (orientation == 1)
		{
			return (unsigned char *) src_p;
		}

	dest_p = (unsigned char *) AllocMemory ((size_t) dest_width * (size_t) dest_height * (size_t) num_channels);

	if (dest_p)
		{
			unsigned char *dest_pixel_p = dest_p;
			int y;

			for (y = 0; y < dest_height; ++ y)
				{
					int x;

					for (x = 0; x < dest_width; ++ x)
						{
							int src_x;
							int src_y;

							switch (orientation)
								{
									case 2:		/* mirrored horizontally */
										src_x = src_width - 1 - x;
										src_y = y;
										break;

									case 3:		/* rotated 180 */
										src_x = src_width - 1 - x;
										src_y = src_height - 1 - y;
										break;

									case 4:		/* mirrored vertically */
										src_x = x;
										src_y = src_height - 1 - y;
										break;

									case 5:		/* transposed */
										src_x = y;
										src_y = x;
										break;

									case 6:		/* needs rotating 90 clockwise */
										src_x = y;
										src_y = src_height - 1 - x;
										break;

									case 7:		/* transversed */
										src_x = src_width - 1 - y;
										src_y = src_height - 1 - x;
										break;

									case 8:		/* needs rotating 90 anticlockwise */
										src_x = src_width - 1 - y;
										src_y = x;
										break;

									default:
										src_x = x;
										src_y = y;
										break;
								}

							memcpy (dest_pixel_p, src_p + (((size_t) src_y * src_width) + src_x) * num_channels, num_channels);
							dest_pixel_p += num_channels;
						}
				}
		}

	return dest_p;
}


/**
 * Check whether a filename has a given extension, ignoring case.
 *
 * @param filename_s The filename to check.
 * @param extension_s The extension without the leading dot e.g. "png".
 * @return <code>true</code> if the filename has the extension, <code>false</code> otherwise.
 */
static bool HasFileExtension (const char * const filename_s, const char * const extension_s)
{
	const char *dot_s = strrchr (filename_s, '.');

	return ((dot_s != NULL) && (Stricmp (dot_s + 1, extension_s) == 0));
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
static bool CreatePlotImageDirectories (const char * const plot_dir_s)
{
	bool success_flag = false;

	if (EnsureDirectoryExists (plot_dir_s))
		{
			if (EnsureSubdirectoryExists (plot_dir_s, S_METADATA_DIR_S, NULL))
				{
					if (EnsureSubdirectoryExists (plot_dir_s, S_THUMBNAILS_DIR_S, NULL))
						{
							char *rotated_dir_s = NULL;

							if (EnsureSubdirectoryExists (plot_dir_s, S_ROTATED_DIR_S, &rotated_dir_s))
								{
									if (EnsureSubdirectoryExists (rotated_dir_s, S_THUMBNAILS_DIR_S, NULL))
										{
											success_flag = true;
										}

									FreeCopiedString (rotated_dir_s);
								}		/* if (EnsureSubdirectoryExists (plot_dir_s, S_ROTATED_DIR_S, &rotated_dir_s)) */

						}		/* if (EnsureSubdirectoryExists (plot_dir_s, S_THUMBNAILS_DIR_S, NULL)) */

				}		/* if (EnsureSubdirectoryExists (plot_dir_s, S_METADATA_DIR_S, NULL)) */

		}		/* if (EnsureDirectoryExists (plot_dir_s)) */
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "EnsureDirectoryExists () failed for \"%s\"", plot_dir_s);
		}

	return success_flag;
}


/**
 * Make sure that a subdirectory exists within a given parent directory.
 *
 * @param parent_dir_s The parent directory.
 * @param subdir_s The name of the subdirectory.
 * @param full_subdir_ss If this is not <code>NULL</code>, then upon success the full path
 * of the subdirectory will be stored here and the caller is responsible for freeing it with
 * FreeCopiedString (). If this is <code>NULL</code>, then the path will be freed.
 * @return <code>true</code> if the subdirectory exists, <code>false</code> otherwise.
 */
static bool EnsureSubdirectoryExists (const char * const parent_dir_s, const char * const subdir_s, char **full_subdir_ss)
{
	bool success_flag = false;
	char *full_subdir_s = MakeFilename (parent_dir_s, subdir_s);

	if (full_subdir_s)
		{
			if (EnsureDirectoryExists (full_subdir_s))
				{
					success_flag = true;
				}
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "EnsureDirectoryExists () failed for \"%s\"", full_subdir_s);
				}

			if (success_flag && full_subdir_ss)
				{
					*full_subdir_ss = full_subdir_s;
				}
			else
				{
					FreeCopiedString (full_subdir_s);
				}

		}		/* if (full_subdir_s) */
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "MakeFilename () failed for \"%s\" and \"%s\"", parent_dir_s, subdir_s);
		}

	return success_flag;
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
	char *last_dot_p = strrchr (full_image_filename_s, '.');

	if (last_dot_p)
		{
			const char * const rotated_s = "_rotated";
			const size_t rotated_length = strlen (rotated_s);

			/*
			 * make sure that we don't look out of the bounds of the
			 * filename string
			 */
			char *start_p = last_dot_p - rotated_length;

			if (full_image_filename_s  < start_p)
				{
					if (strncmp (start_p, rotated_s, rotated_length) == 0)
						{
							rotated_flag = true;
						}
				}

		}		/* if (last_dot_p) */

	return rotated_flag;
}


