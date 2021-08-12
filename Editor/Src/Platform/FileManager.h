#ifndef _FILE_MANAGER_H
#define _FILE_MANAGER_H

/*
 *
 * The purpose of the file manager is to track files/directiories
 * within a mount point.
 *
 * For the asset system, we have two types of files:
 * - Asset File
 * - META File
 *
 * A meta file contains meta data any given asset. Meta files are
 * stored within the hidden directory ".internal" in a project.
 *
 * A project will map the following files automatically:
 * - Project Root directory (project data)
 * - .internal directory (hidden project files)
 * - Engine data directory (engine runtime data)
 *
 * The project root will contain asset files, while the internal
 * directory contains the META files.
 *
 * Asset Files do not keep a list of "virtual names" or file names
 * to other dependent assets. Instead, Assets will refer to each other
 * with a persistent GUID. A GUID is mapped to a file name by the file manager.
 *
 * There are two major benefits to using a file manager:
 * 1. Tracking file updates on disc and and updating their runtime version.
 *    This is often called "hotreloading"
 * 2. Getting the following mappings:
 *    - GUID to Filename
 *    - GUID to Filename
 *    - Virtual Name to GUID
 *    - Virtual Name to Filename
 *
 * This is where design gets tricky. Ideally, you want the Platform file manager
 * to remain unaware of things like "assets" or "GUIDs" so it is tricky to find
 * where to distinguish between where the File Manager begins and ends and how
 * the Asset Manager interacts with the File Manager.
 *
 * Here is going to be the current approach:
 * 1. The purpose File Manager is to ONLY track files, and ping the engine if a file updates.
 * 2. It will maintain several storage containers:
 *    - Hashtable the maps filenames to file information (i.e. last update)
 *    - Tree structure of File IDs so that external utilities can traverse the file tree.
 *      This is useful for visualization (i.e. File Browsers).
 * 3. For each mount point, the File Manager will fire an APC to update the tree listings
 *    and ping the engine if a file has been updated. Events will work similar to events
 *    for the windowing API.
 *
 * It will be the job of an asset manager to maintain mapping of Filenames <-> GUIDs <-> Virtual
 * names. It will also be the job of the asset manager to actually reload assets when the event
 * notification is triggered.
 *
 */


#endif //_FILE_MANAGER_H
