# AutoProducer
Parses a formatted video script into a .kdenlive file, for more efficient video editing.
Once the video file has been generated from the script, you can open it in the Kdenlive video editor, and make minor adjustments.
This removes the bulk of the work, i.e., adding all the clips and audio files in order and at the correct position, so that you can focus on the smaller/more important details.

AutoProducer.exe is called from the command line in the following format:
```
AutoProducer.exe "video_script.txt" "output_filename"
```
The format for the script file can be seen in example_script.txt.
Aside from the small amount of generation settings to declare at the top, the video script and the clips to play throughout are all that is needed in the script file, making it easy and simple to make.
