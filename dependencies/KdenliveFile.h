#ifndef KDENLIVEFILE_H
#define KDENLIVEFILE_H

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "tinyxml2.h"


// FILE MANAGEMENT
/** Creates an input stream at the file location
 */
std::ifstream openInputFile(const std::string &file_path);
/** Creates an output stream at the file location
 */
std::ofstream openOutputFile(const std::string &filePath);
/** Returns the entire file as a string
 */
std::string readEntireFile(std::ifstream &input_file);


// External data types
typedef int ClipId;
typedef int TrackId;
typedef int TrackEntryId;


// Wrapper class for XMLDocument, specifically for .kdenlive files
class KdenliveFile{
    public:
    // Data types that I don't want cluttering the global namespace
    enum TrackType{
        AUDIO,
        VIDEO,
    };

    private:
    // Internal data types
    enum EntryType{
        CLIP,
        BLANK,
    };
    struct TrackEntry{
        EntryType entry_type;
        float length;
        float start_offset;
    };


    public:
    // CONSTRUCTORS 
    /** Constructs an empty Kdenlive file, with 0 video tracks and 0 audio tracks.
     *  The empty file is generated by using a new file generated by Kdenlive, and then removing the tracks. 
     *  NOTE: At least one track needs to be added to create a valid file.
     */
    KdenliveFile();

    // SETTERS
    /** Specifies the profile of the video.
     *  NOTE: Kdenlive may only allow certain profile presets, so the profile you specify here may be overwritten by Kdenlive. 
     * 
     *  @param framerate specifies the framerate of the video
     *  @param width specifies the width, in pixels, of the video
     *  @param length specifies the length, in pixels, of the video
     */
    void SetProfile(const int framerate, const int width, const int height);
    /** Adds a new track to the file, either video or audio
     *  Returns a TrackId, which is used to add clips to the new track.
     */
    TrackId AddTrack(const TrackType track_type);
    /** Adds a clip to the project bin.
     *  Returns a ClipId, which is used to add the new clip to a track.
     *  This must be called before a clip can be added using AddClipToTrack().
     */
    ClipId AddClipToBin(const std::string &clip_path);
    /** Adds a blank space with the given length to the end of the track.
     *  Returns a TrackEntryId, which is used to modify the entry later, if needed.
     */
    TrackEntryId AddBlankToTrack(const TrackId track_id, const float length);
    /** Adds a clip from the bin with the given length and starting offset to the end of the track.
     *  Returns a TrackEntryId, which is used to modify the entry later, if needed.
     */
    TrackEntryId AddClipToTrack(const TrackId track_id, const ClipId clip_id, const float clip_length, const float clip_start_offset = 0);
    /** Adds a fade filter to the given entry, on the given track.
     *  
     *  NOTE: This does not currently affect clips placed on an audio track.
     * 
     *  @param fade_in_time specifies how long the fade will last at the beginning of the entry.
     *  @param fade_out_time specifies how long the fade will last at the end of the entry.
     */
    void FadeClip(const TrackId track_id, const TrackEntryId entry_id, const float fade_in_time, const float fade_out_time);
    
    // GETTERS
    /** Returns the length all entries on the track
     */
    float GetTrackLength(const TrackId track_id);
    /** Returns the file as a string, which can then be saved to a file.
     */
    std::string ToString() const;
    /** Saves the KdenliveFile to the given directory.
     *  If no output filepath is specified, then it will save the file to current directory.
     */
    void SaveToFile(const std::string &file_name, const std::string &output_filepath = "") const;


    private:
    // HELPERS
    tinyxml2::XMLElement* CreatePropertyElement(const char* name, const char* value);
    tinyxml2::XMLElement* AddPropertyElement(tinyxml2::XMLElement* element_to_add_to, const char* name, const char* value);
    tinyxml2::XMLElement* CreateEntryElement(const float in, const float out, const char* producer);
    tinyxml2::XMLElement* AddEntryElement(tinyxml2::XMLElement* element_to_add_to, const float in, const float out, const char* producer);
    tinyxml2::XMLElement* CreateBlankElement(const float length);
    tinyxml2::XMLElement* AddBlankElement(tinyxml2::XMLElement* element_to_add_to, const float length);
    tinyxml2::XMLElement* CreateTrackElement(const char* producer);
    tinyxml2::XMLElement* AddTrackElement(tinyxml2::XMLElement* element_to_add_to, const char* producer);
    tinyxml2::XMLElement* CreateFilterElement(const char* id, const float in, const float out);
    tinyxml2::XMLElement* AddFilterElement(tinyxml2::XMLElement* element_to_add_to, const char* id, const float in, const float out);
    tinyxml2::XMLElement* CreateChainElement(const char* id, const char* resource);
    tinyxml2::XMLElement* AddChainElement(tinyxml2::XMLElement* element_to_add_to, const char* id, const char* resource, tinyxml2::XMLElement* insert_after = nullptr);
    tinyxml2::XMLElement* CreatePlaylistElement(const char* id);
    tinyxml2::XMLElement* AddPlaylistElement(tinyxml2::XMLElement* element_to_add_to, const char* id, tinyxml2::XMLElement* insert_after = nullptr);
    tinyxml2::XMLElement* CreateTractorElement(const char* id);
    tinyxml2::XMLElement* AddTractorElement(tinyxml2::XMLElement* element_to_add_to, const char* id, tinyxml2::XMLElement* insert_after = nullptr);

    void AddElementToTopOfRoot(tinyxml2::XMLElement* element);
    void AddElementToRoot(tinyxml2::XMLElement* element);

    tinyxml2::XMLElement* FindPlaylistElement(const char* playlist_id) const;
    tinyxml2::XMLElement* FindTractorElement(const char* tractor_id) const;
    tinyxml2::XMLElement* FindPlaylistEntry(const char* playlist_id, const TrackEntryId entry_index);

    std::string FindDocUUID();

    void DeletePreExistingTracks();

    // PRIVATE VARIABLES
    // Elements
    tinyxml2::XMLDocument xml_doc;
    tinyxml2::XMLElement* root;
    tinyxml2::XMLElement* profile;
    tinyxml2::XMLElement* main_producer;
    tinyxml2::XMLElement* timeline_tractor;
    tinyxml2::XMLElement* main_bin;
    tinyxml2::XMLElement* final_tractor;
    tinyxml2::XMLElement* last_added_root_element;
    // Keeping track of important data
    int chain_count;
    int track_count;
    int filter_count;
    std::vector<float> track_lengths;
    std::vector<std::vector<TrackEntry>> track_entries;
};


#endif