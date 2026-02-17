
function AccessionChanged (accession_element)
{
	console.log (accession_element.value);
}

/**
 * Check the number is odd
 *
 * @param {Int} n - Number input.
 */
function isOdd(n) 
{
	return Math.abs(n % 2) == 1;
}

const S_DEBOUNCE_DELAY = 200;

const KeywordSearchGrassroots = Debounce (RealKeywordSearchGrassroots, S_DEBOUNCE_DELAY);

const S_GRASSROOTS_SERVER_URL = "http://localhost:2000/grassroots/"

const S_DJANGO_SERVER_URL = "http://localhost:8000/"


const S_BACKEND = "public_backend"


const S_PHENOTYPE_HEADER = "data-phenotype";


const S_MIN_PHENOTYPE_SUFFIX = "-min";

const S_MAX_PHENOTYPE_SUFFIX = "-max";


/**
 * Ajax search services for searching Treatments and Measured Variables
 *
 * @param {String} type - Define Treatments or Measured Variables.
 */
async function RealKeywordSearchGrassroots (query, facet_type) 
{
	console.log ("RealKeywordSearchGrassroots: " + query);

	let input_tail = "";

	if (isOdd((query.match(/\"/g) || []).length)) 
		{
			input_tail = "\"";
		}
	
	if (((query.match(/\"/g) || []).length) == 0) 
		{
			input_tail = "*";
		}

	console.log ("query: " + query + " tail: " + input_tail);

	if (query.length > 1) 
		{

			let submit_json = 
				{
					"services": [{
						"start_service": true,
						"so:name": "Search Grassroots",
						"parameter_set": {
							"level": "simple",
							"parameters": [{
								"param": "SS Keyword Search",
								"current_value": query + input_tail
							}, {
								"param": "SS Facet",
								"current_value": facet_type
							}]
						}
					}]
				};

			let req_body = JSON.stringify (submit_json);

		//	console.log ("req_body: " + req_body);

		
			document.getElementById("loader").style.visibility = "visible";
			//document.getElementById("loader").innerHTML = "Loading...";

			const response = await fetch (S_GRASSROOTS_SERVER_URL + "/" + S_BACKEND, {
				method: "POST",
				body: req_body,
			});

	//		console.log ("req_body: " + req_body);

			if (response.ok) 
				{
					let res_json = await response.json ();
	//				console.log ("res_json: " + JSON.stringify (res_json));
					LoadKeywordSearchResults (res_json);
				}
		 	else 
				{
					console.log ("response status: " + response.status);
				}

			document.getElementById("loader").style.visibility = "hidden";
			//document.getElementById("loader").innerHTML = "";

		} 
	else 
		{
			document.getElementById ("phenotypes_tbody").innerHTML = "";
		}
}


function GetHitsFromJSON (response_json)
{
	let hits = null;
	const results = response_json.results;

	if (results) 
		{
			if (Array.isArray (results)) 
				{
					if (results.length == 1) 
						{
							hits = results [0].results;
						}
				}
		}
		
	return hits;
}


const S_SCALE_CLASS_DATA_KEY = "data-scale-class"; 

function LoadKeywordSearchResults (response_json) 
{
	const hits = GetHitsFromJSON (response_json);

	if (hits)
		{
			let table_body = "";
			
			for (let i in hits) 
				{
					const hit = hits [i];
					const data = hit.data;
				
					console.log ("hit [" + i + "]: " + JSON.stringify (hit));
				
					let tr = "<tr onclick=\"SelectRow (this)\"";

					if (i % 2 == 1) 
						{
							tr += " class=\"odd\"";
						}
				
						tr += " data-var-name=\"" + data.variable ["so:name"]+ "\"";
						
						if (data.scale ["so:name"])
							{
								tr += " " + S_SCALE_CLASS_DATA_KEY + "=\"" + data.scale ["so:name"] + "\"";
							}
							
						tr += ">\n<td>" + data ["so:name"] + "</td>\n" +
						"<td>" + data.variable ["so:name"] + "</td>\n" +
						"<td>" + data.trait ["so:name"] + "</td>\n" +
						"<td>" + data.trait ["so:description"] + "</td>\n";
						"<tr>\n";

					table_body += tr;
				}

			
			document.getElementById ("phenotypes_tbody").innerHTML = table_body;
			let d = document.getElementById ("phenotypes_dialog");
			
			if (d)
				{
					d.showModal ();
				}
			else
				{
					console.log ("no phenotypes_dialog");
				}
		}

}



/**
 * Add a given Phenotype to the list of selected ones
 */
function SelectRow (table_row) 
{
//	console.log ("selected table_row: ");
//	console.log (table_row.innerHTML);

	let selected_variable = table_row.cells [1].innerHTML;
//	console.log ("variable: " + selected_variable);

	let trait_name = table_row.cells [2].innerHTML;
//	console.log ("trait_name: " + trait_name);

	let trait_description = table_row.cells [3].innerHTML;
//	console.log ("trait_description: " + trait_description);

	let phenotypes_list = document.getElementById ("selected_phenotypes");

	let scale_class = table_row.getAttribute (S_SCALE_CLASS_DATA_KEY)

	console.log ("var name: " + selected_variable);

	console.log ("scale_class: " + scale_class);

	/* 
	 * Check to see if the phenotype is already on the list 
	 */
	const var_li = document.querySelector(`#selected_phenotypes li[data-var-name="${selected_variable}"]`);
	if (var_li !== null) 
		{	
			console.log (selected_variable + " is already on list");
		}
	else
		{
			let phenotype_entry = document.createElement ("li");

			/*
				Create the delete button
			*/
			let remove_button = document.createElement ("input");
			remove_button.type = "image"
			remove_button.setAttribute ("src", "/grassroots/images/aiss/delete");
			
			let v = RemoveTags (selected_variable);
			remove_button.setAttribute ("onclick", "RemoveSelectedPhenotype (this.parentElement)");
			remove_button.setAttribute ("title", "Remove " + v + " from selected phenotypes");

			phenotype_entry.appendChild (remove_button);

			phenotype_entry.setAttribute ("title", RemoveTags (trait_description));
			phenotype_entry.setAttribute ("data-var-name", v);

			phenotype_entry.appendChild (document.createTextNode (v));


			if (scale_class)
				{
					/* Add the min and max boxes so the user can specify the range of values */
					
					let limits = document.createElement ("span");

					limits.setAttribute ("class", "limits");
					
					phenotype_entry.appendChild (limits);
					
					AddNumericInput (limits, v, "Min: ", S_MIN_PHENOTYPE_SUFFIX);
					AddNumericInput (limits, v, "Max: ", S_MAX_PHENOTYPE_SUFFIX);
				}

	
			phenotypes_list.appendChild (phenotype_entry);
		}



	if (table_row.classList.contains ("selected"))
		{
			table_row.classList.remove ("selected");
		}
	else
		{
			table_row.classList.add ("selected");
		}
}


function AddNumericInput (parent_element, input_id, label_text, id_suffix)
{
	let box = document.createElement ("input");
	const box_id = input_id + id_suffix;
	
	box.setAttribute ("id", box_id);
	box.setAttribute ("name", box_id);
	box.setAttribute ("type", "text");
	box.setAttribute ("inputmode", "numeric");
	box.setAttribute ("pattern", "/[\d]*[\.]*[\d]+/");	
	
	
	let l = document.createElement ("label");
	l.setAttribute ("for", box_id);
	l.appendChild (document.createTextNode (label_text));
	
	parent_element.appendChild (l);
	parent_element.appendChild (box);
	
	return box
}

function RemoveSelectedPhenotype (list_entry)
{
	let phenotypes_list = list_entry.parentElement;
	const selected_variable = list_entry.getAttribute ("data-var-name");
	console.log ("list_entry " + list_entry);
	console.log ("phenotypes_list " + phenotypes_list);

	phenotypes_list.removeChild (list_entry);

	if (selected_variable)
		{
			const table_row = document.querySelector(`#phenotypes_tbody tr[data-var-name="${selected_variable}"]`);

			if (table_row)
				{
					table_row.classList.remove ("selected");
				}
		}
}


/**
 * Instead of running a function every time, add a small delay before
 * running. If the user is typing in a search term this will then only 
 * call the server once they've paused typing rather than on every 
 * keystroke.
 * 
 * @param callback_fn The function to run after the given time of inactivity.
 * @param wait_time The time in milliseconds to use as the inactivity time.
 */
function Debounce (callback_fn, wait_time) 
{
	let timer;

	return function (...args) 
		{
			// cancel the timer
			clearTimeout (timer);

			// set a new timer
			timer = setTimeout (() => 
				{
					// run the callback function with its original args
					callback_fn.apply (this, args);
				}, 
				wait_time
			);
		};
}


function KeywordSearchGrassrootsHandler (event)
{
	// Cancel the default action

	var query = event.target.value;

	const facet = "Measured Variable";

	if (event.key === "Enter")
		{
			// Do the search immediately
			event.preventDefault ();
			RealKeywordSearchGrassroots (query, facet);
		}
	else
		{
			Debounce (RealKeywordSearchGrassroots, S_DEBOUNCE_DELAY, query, facet);
		}
}




async function SearchStudies ()
{
	console.log ("SearchStudies");


	let accessions = "";

	/** 
	 * phenotypes is a json array where each object 
	 * is of the form 
	 * { 
	 *.  "name": phnotype_variable_name,
	 *   "min": min_value,
	 *   "max": max_value
	 *. }
	 * 
	 * where name is required and min and max are optional.
   */	
	let phenotypes_json = [];
	
	/*
	 * Get the accessions
	 */
	let el = document.getElementById ("accession_text");
	if (el)
		{
			accessions = el.value;					
		}

	/*
	 * Get the phenotypes
	 */
	const phenotype_items = document.querySelectorAll ('#selected_phenotypes li');
	
	let thead = document.getElementById ("studies_results_table_header_row");

/*
	let study_table_rows = document.getElementById ("studies_results_table").rows;
	let first_row = study_table_rows [0].children;
	let i = first_row.length - 1; 
	
	for ( ; i >= 0; -- i)
		{
			if (first_row.item (i).getAttribute (S_PHENOTYPE_HEADER))
				{
					let j = study_table_rows.length - 1;
					
					for ( ; j >= 0; -- j)
						{
							//console.log ("removing " + i  + "," + j + ": " + ][i].innerHTML);

							let table_row = study_table_rows [j];
							table_roww.deleteCell (i);
						}
				}
		}
	*/
	
	let cells = document.querySelectorAll ("#studies_results_table_header_row th[" + S_PHENOTYPE_HEADER + "], #studies_results_table_header_row td[" + S_PHENOTYPE_HEADER + "]");
	cells.forEach (function (cell) {
		cell.remove ();
	});
	
	
	//document.getElementById ("studies_tbody").innerHTML = "";

	console.log ("adding " + phenotype_items.length  + " phenotypes");

	if (phenotype_items)
		{
			const final_index = phenotype_items.length - 1;
			let added_entry = false;
			
			for (let i = 0; i <= final_index; ++ i) 
				{
					const var_name = phenotype_items.item (i).getAttribute ("data-var-name");				
					
					if (var_name) 
						{
							let min_limit = null;
							let max_limit = null;
							
							console.log (i + ": adding " + var_name);

							
							el = document.getElementById (var_name + S_MIN_PHENOTYPE_SUFFIX);
							if (el)
								{
									min_limit = el.value;
								}

							el = document.getElementById (var_name + S_MAX_PHENOTYPE_SUFFIX);
							if (el)
								{
									max_limit = el.value;
								}
							
							AddPhenotype (phenotypes_json, var_name, min_limit, max_limit);
								
							/* Add phenotype as table column header */
							let th = document.createElement ("th");
							
							const trait_description = phenotype_items.item (i).getAttribute ("title");
							if (trait_description)
								{
									th.setAttribute ("title", trait_description);
								}
								
							th.appendChild (document.createTextNode (var_name));					
							th.setAttribute ("id", var_name);
							th.setAttribute (S_PHENOTYPE_HEADER, S_PHENOTYPE_HEADER);
							thead.appendChild (th);
	
						}
						
				}
		}

	let submit_json = 
		{
			"services": 
				[
					{
						"start_service": true,
						"so:name": "Search Field Trials",
						"parameter_set": 
							{
								"level": "wizard",
								"parameters": 
									[
										{
											"param": "ST Search Study Accessions",
											"current_value": accessions
										},
										{
											"param": "ST Search Study Phenotypes",
											"current_value": phenotypes_json,
										},
										{
											"param": "The level of data to get for matching Studies",
											"current_value": "Metadata",
										}
									]
							}
					}
				]
			};

	let req_body = JSON.stringify (submit_json);

//	console.log ("req_body: " + req_body);

	let loader = document.getElementById ("loader");
	
	if (loader)
		{
			loader.style.visibility = "visible";
		}
		
	const response = await fetch (S_GRASSROOTS_SERVER_URL + "/" + S_BACKEND, {
		method: "POST",
		body: req_body,
	});

	if (response.ok) 
		{
			let res_json = await response.json ();	
			console.log ("res_json: " + JSON.stringify (res_json));
			LoadStudySearchResults (res_json);
		}
	else 
		{
			console.log ("response status: " + response.status);
		}

	if (loader)
		{
			loader.style.visibility = "hidden";
		}


}


/**
  * Add a phenotype to the phenotypes json array 
 */
function AddPhenotype (phenotypes_json, variable_name, min_value, max_value)
{
	let phenotype_json = {};
	
	phenotype_json ["name"] = variable_name;
	
	if (min_value)
		{
			phenotype_json ["min"] = min_value;		
		}

	if (max_value)
		{
			phenotype_json ["max"] = max_value;		
		}

		phenotypes_json.push (phenotype_json);
}


function LoadStudySearchResults (response_json) 
{
	const hits = GetHitsFromJSON (response_json);

	if (hits)
		{
			let table_body = "";
			let table_header_row = document.getElementById ("studies_results_table_header_row");

			console.log ("table_header " + table_header_row.innerHTML);

			
			for (let i in hits) 
				{
					const hit = hits [i];
					const data = hit.data;
				
					let tr = "<tr";

					if (i % 2 == 1) 
						{
							tr += " class=\"odd\"";
						}
				
				
					tr += ">" + 
						"<td> " + "<a href=\"" + S_DJANGO_SERVER_URL + "/fieldtrial/study/" + data._id ["$oid"] + "\" target=\"_blank\">" + data ["so:name"] + "</a> </td>\n" +
						"<td> " + data ["so:name"] + " </td>\n";

					
					/* loop over the phenotypes */
					for (let j = 0; j < table_header_row.cells.length; ++ j)
						{
							console.log ("index " + j + ": " + table_header_row.cells [j].innerHTML);
							const phenotype_th = table_header_row.cells [j];
							const phenotype_name = phenotype_th.getAttribute ("id");
							
							if (phenotype_name)
								{
									const study_phenotype = data.phenotypes [phenotype_name];
									
									tr += "\n<td " + S_PHENOTYPE_HEADER + "=\"" + S_PHENOTYPE_HEADER + "\"> ";
									
									if (study_phenotype)
										{
											const stats = study_phenotype.statistics;
											
											if (stats)
												{
													tr += " <ul class=\"stats\">\n";
													
													let v = stats ["stato:0000150"];													
													if (v)
														{
															const n = Number.parseFloat (v).toFixed (3);
															tr += "<li>Min: " + n + "</li>\n";
														}
													
													v = stats ["stato:0000401"];
													if (v)
														{
															const n = Number.parseFloat (v).toFixed (3);
															tr += "<li>Mean: " + n + "</li>\n";
														}
																											
													v = stats ["stato:0000151"];
													if (v)
														{
															const n = Number.parseFloat (v).toFixed (3);
															tr += "<li>Max: " + n + "</li>\n";
														}													
														
													tr += " </ul>\n";
												}								
			
										}
				
									tr += " </td>\n";
				
									
								}
							
							
						}
					
					tr +=	" <tr>\n";
		
					table_body += tr;
				}

			
			document.getElementById ("studies_tbody").innerHTML = table_body;
		}

}


/**
 * Janghou's answer on how to strip html from https://stackoverflow.com/a/17980070
 */
function RemoveTags (html_source)
{
   var tmp = document.implementation.createHTMLDocument ("New").body;
   tmp.innerHTML = html_source;
   return tmp.textContent || tmp.innerText || "";
}


function GetTableHeaderForCell (td)
{
	let tr = td;
	

}
