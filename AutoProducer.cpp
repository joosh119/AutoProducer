#include <sstream>
#include "dependencies/KdenliveProject.h"


// g++ *.cpp dependencies/*.cpp -g -o AutoProducer.exe
// AutoProducer.exe "video_script.txt" "output_file_name"
// AutoProducer.exe "example_script.txt"

const std::string DEFAULT_FILE_NAME = "generated_file";
const std::string DEFAULT_OUTPUT_PATH = "out";


struct Element{
    const std::string element_name; 
    void (*parse_func)(std::ifstream &script);
};


void parseMedia(std::ifstream &script);
void parseProfile(std::ifstream &script);
void parseGenerationSettings(std::ifstream &script);
void parseAudio(std::ifstream &script);
void parseScript(std::ifstream &script);

const Element MEDIA_ELEMENT = {"media", parseMedia};
const Element PROFILE_ELEMENT = {"profile", parseProfile};
const Element GENERATION_SETTINGS_ELEMENT = {"gen", parseGenerationSettings};
const Element AUDIO_ELEMENT = {"bg_audio", parseAudio};
const Element SCRIPT_ELEMENT = {"script", parseScript};

const int ELEMENT_COUNT = 5;
const Element elements[ELEMENT_COUNT] = {
    MEDIA_ELEMENT,
    PROFILE_ELEMENT,
    GENERATION_SETTINGS_ELEMENT,
    AUDIO_ELEMENT,
    SCRIPT_ELEMENT,
};


struct SegmentClip{
    float rel_pos = 0;
    float rel_len = 0;
    std::string name;
};
struct Segment{
    std::vector<SegmentClip> clips;
    std::vector<int> word_counts;
};


// Global parsing data
int line_number = 0;
int current_element = 0;
KdenliveProject video;
std::vector<std::string> media_paths;
float clip_fade_time = 0;
float wpm = 150;
float background_audio_end_time = 0;
float segment_end_time = 0;
bool include_audio = false;
int current_audio_num = 1;
Segment current_segment;


void parsingError(const std::string &error, const int line_number){
    std::cerr << error << " [at line " << line_number << "]\n";
    exit(-1);
}

std::string getClipName(const std::string &line, const size_t s_pos){
    if(size_t s_i = line.find_first_of("c[") == 0){
        size_t e_i = line.find(']');
        if(e_i == std::string::npos)
            parsingError("Closing bracket not found.", line_number);
        
        std::string clip_name = line.substr(2, e_i - 2);
        return clip_name;
    }
    
    return "NOT_FOUND";
}

Segment ReadClips(const std::string &line){
    Segment new_segment;
    
    // Find the number of cuts, and store their indexes
    std::vector<size_t> cut_indexes = {0};

    size_t cut_i = -1;
    while( (cut_i = line.find("->", cut_i+1)) != std::string::npos ){
        cut_indexes.push_back(cut_i);
    }

    cut_indexes.push_back(line.size()+1);

    const float max_rel_len = 1.0F / (cut_indexes.size() - 1);

    // Iterate between each cut operator
    for(int i = 0; i < cut_indexes.size() - 1; i++){
        const float start_rel_pos = max_rel_len * i;

        // Get the substr
        const std::string& cut_str = line.substr(cut_indexes[i], cut_indexes[i+1] - cut_indexes[i] - 1);
        
        // Find each clip
        float rel_length = max_rel_len;
        float rel_pos = start_rel_pos;
        size_t clip_i = -1;
        while( (clip_i = cut_str.find("c[", clip_i+1)) != std::string::npos ){
            size_t e_clip_i = cut_str.find(']', clip_i);
            if( e_clip_i == std::string::npos)
                parsingError("Closing bracket not found.", line_number);
            
            // Find name of clip
            const std::string& name = cut_str.substr(clip_i+2, e_clip_i - clip_i - 2);

            // Add clip to segment
            new_segment.clips.push_back({rel_pos, rel_length, name});

            // Find overlap operator
            if( cut_str.find("><", e_clip_i) != std::string::npos ){
                rel_length /= 2;
                rel_pos += rel_length/2;
            }
        }
    }

    return new_segment;
}

void AddSegmentToVideo(const Segment &segment){
    //Get total word count
    int total_word_count = 0;
    for(const int count : current_segment.word_counts)
        total_word_count += count;
    
    if(total_word_count == 0)
        return;

    const float segment_length = total_word_count / wpm * 60.0F;

    // Add clips
    for(const SegmentClip &clip : segment.clips){
        float exact_position = segment_end_time + (segment_length * clip.rel_pos);
        float exact_length = segment_length * clip.rel_len;
        
        video.CreateClipOnVideoTrack(exact_position, clip.name, exact_length);
    }

    // Add audio
    float curr_pos = segment_end_time;
    for(const int text_word_count : segment.word_counts){
        float text_length = text_word_count / wpm * 60.0F;

        const std::string audio_str = std::to_string(current_audio_num);
        
        if(include_audio)
            video.CreateClipOnAudioTrack(curr_pos, audio_str, text_length);

        current_audio_num++;
        curr_pos += text_length;
    }


    segment_end_time += segment_length;
}


// Don't judge me for this
#define PARSE_LINE_START    std::string line;                       \
                            while( std::getline(script, line) ){    \
                            line_number++;                          \
                            if( line.find_first_of("//") == 0)      \
                            continue;                                  
#define PARSE_ELEMENT_CHECK if(line.find_first_of("<") == 0){                                                                                               \
                            if(line.find_first_of('/' + elements[current_element].element_name + '>') == 1){                                                \
                            break;                                                                                                                         \
                            }                                                                                                                               \
                            else{                                                                                                                           \
                                parsingError("The element \"" +  elements[current_element].element_name + "\" does not terminate properly.", line_number);  \
                            }                                                                                                                               \
                            }                                                                                                                               
#define PARSE_ELEMENT_ARG   size_t eq_index = line.find('=');                                       \
                            if(eq_index == std::string::npos)                                       \
                            parsingError("No equal sign given for variable.", line_number);         \
                            const std::string var = line.substr(0, eq_index);                     \
                            if(eq_index == line.size() - 1 )                                        \
                            parsingError("No argument given for \"" + var + " \"", line_number);    \
                            const std::string arg = line.substr(eq_index+1, line.size()-eq_index);
#define PARSE_LINE_END      end_this_line:; \
                            }


void parseMedia(std::ifstream &script){
    PARSE_LINE_START
    PARSE_ELEMENT_CHECK
    
    // Save each argument
    media_paths.push_back(line);

    PARSE_LINE_END
}

void parseProfile(std::ifstream &script){
    // Variables to set
    float fps = -1;
    int width = -1;
    int height = -1;


    PARSE_LINE_START
    PARSE_ELEMENT_CHECK
    PARSE_ELEMENT_ARG

    // Set the variable, if valid
    if(var == "fps"){
        fps = std::stof(arg);
    }
    else if(var == "width"){
        width = std::stoi(arg);
    }
    else if(var == "height"){
        height = std::stoi(arg);
    }
    else
        parsingError("The variable\"" + var + "\" is not valid.", line_number);

    PARSE_LINE_END


    // Set the variables
    video.SetProfile(fps, width, height);
}

void parseGenerationSettings(std::ifstream &script){
    PARSE_LINE_START
    PARSE_ELEMENT_CHECK
    PARSE_ELEMENT_ARG
    
    // Set the variable, if valid
    if(var == "wpm"){
        wpm = std::stof(arg);
    }
    else if(var == "script_audio"){
        include_audio = (arg[0] == 't');
    }
    else if(var == "clip_fade"){
        clip_fade_time = std::stof(arg);
    }
    
    else
        parsingError("The variable\"" + var + "\" is not valid.", line_number);

    PARSE_LINE_END
}

void parseAudio(std::ifstream &script){
    PARSE_LINE_START
    PARSE_ELEMENT_CHECK
    
    // Check that the syntax is correct
    if(size_t s_i = line.find_first_of("a[") == 0){
        size_t e_i = line.find(']');
        if(e_i == std::string::npos)
            parsingError("Closing bracket not found.", line_number);
        
        std::string audio_name = line.substr(2, e_i - 2);

        s_i = line.find('[', e_i);
        if(s_i == std::string::npos)
            parsingError("Length argument not found.", line_number);
        e_i = line.find(']', s_i);
        if(e_i == std::string::npos)
            parsingError("Closing bracket not found.", line_number);

        float length = std::stof( line.substr(s_i+1, e_i-s_i-1) );

        // Place audio onto track
        video.CreateClipOnAudioTrack(background_audio_end_time, audio_name, length);
        background_audio_end_time += length;
    }
    else
        parsingError("Incorrect syntax for audio element.", line_number);
    


    PARSE_LINE_END
}

void parseScript(std::ifstream &script){
    bool text_split = false;
    
    PARSE_LINE_START
    PARSE_ELEMENT_CHECK
    //std::cout << "Line: " << line_number << "\n";
    // Check for a clip
    if(size_t s_i = line.find_first_of("c[") == 0){
        // Add the previous segment first
        AddSegmentToVideo(current_segment);

        current_segment = ReadClips(line);

        // Start word counting at zero
        current_segment.word_counts.push_back(0);
        text_split = false;
    }

    // Read the script
    else{
        // Check for empty line
        if(line == ""){
            text_split = true;
            continue;
        }

        if(text_split){
            // If we haven't started counting words yet, don't split into a new word count
            if(current_segment.word_counts.back() > 0)
                current_segment.word_counts.push_back(0);
            text_split = false;
        }
        
        //Counts the words in the script
        std::stringstream stream(line);
        std::string word;
        while(stream >> word){
            current_segment.word_counts.back()++;
        }
    }

    PARSE_LINE_END


    AddSegmentToVideo(current_segment);
}


void parseScriptFile(std::ifstream &script){
    // Ignore leading whitespaces
    script >> std::ws;

    // Start parsing, line by line
    PARSE_LINE_START
        
        // Check for an element at the start of the line
        if(line.find_first_of("<") == 0){
            // Find the type of element
            std::size_t f_index = line.find('>');
            
            if( f_index == std::string::npos)
                parsingError("No closing bracket found.", line_number);
            else{
                const std::string element_str = line.substr(1, f_index-1);

                // Find the element to parse
                for(int i = 0; i < ELEMENT_COUNT; i++){
                    
                    if(element_str == elements[i].element_name){
                        current_element = i;
                        elements[i].parse_func(script);

                        goto end_this_line;
                    }
                }
                
                // If the element doesn't exist, throw an error
                parsingError("The element \"" + element_str + "\" does not exist.", line_number);
            }

        }

    PARSE_LINE_END
}



int main(int argc, char** argv){
    // Get command line arguments
    if(argc < 2 || argc > 3){   
        std::cerr << argc - 1 << " arguments given; expected 1 or 2.\n";
        return -1;
    }

    const std::string &script_path = argv[1];
    std::string file_name = DEFAULT_FILE_NAME;
    std::string output_path = DEFAULT_OUTPUT_PATH;
    if(argc >= 3)
        file_name = argv[2];

    // Open the file
    std::ifstream script = openInputFile(script_path);

    std::cout << "Parsing file. \n";

    // Parse the script
    parseScriptFile(script);

    std::cout << "Save to file. \n";

    // Save the video to the file path
    video.SaveToFile(media_paths, file_name, output_path);

    std::cout << "Finished. \n";

    return 0;
}