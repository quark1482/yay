# YAY - yet another yt downloader
Sample application created in Qt/C++ as a proof of concept on how to download
and manipulate videos from YT.


Features
--------

* Downloads constant/adaptive-bitrate videos from YT.
* Uses a hidden browser engine to inject and execute JS code.
* Uses FFmpeg library to MUX and cut streams.


Dependencies
------------

* [FFmpeg](https://www.ffmpeg.org/)

I've used [vcpkg](https://vcpkg.io/) to download and build FFmpeg:\
`vcpkg install ffmpeg:x64-windows`

CMake's *find_package()* does wonders to simplify the usage after installation:\
`find_package(FFmpeg REQUIRED COMPONENTS AVCODEC AVFORMAT AVUTIL)`


ToDo's
------

- [ ] Optionally process locally stored media
- [ ] Download other protected videos
- [ ] Implement authentication
- [ ] Download subtitle tracks when available
- [ ] Hardcode downloaded subtitles in the processed media
- [ ] Reencode frames for better cutting precision
- [ ] Reencode audio streams to MP3


Getting involved
----------------

I'm open to criticism and suggestions. Feel free to [send me](mailto:quark1482@protonmail.com?subject=[GitHub]%20YAY%20downloader) your comments. :slightly_smiling_face:

To code purists, I advocate for standards and code conventions. But I also love\
structured code **with all my heart**.


_Notice_
--------

If for some weird reason, at this point you are still not sure about what does\
_YT_ mean, check the first lines of the file [ytscraper.cpp](https://github.com/quark1482/yay/blob/main/src/ytscraper.cpp) to confirm.


<br><br>
_This README file is under construction._
