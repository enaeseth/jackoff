Jackoff
=======

Jackoff is a simple utility that records audio off of a [JACK][jack] source to
a file, using any of the encodings supported by [libsndfile][libsndfile].

Jackoff is heavily based on [Rotter][rotter], another program that allows you
to record audio from JACK and save it to file. The main differences are:

  - No file rotation. Rotter starts writing to a different file every hour, but
    Jackoff does not include this behavior. You pass it the name of a file you
    wish to write to and Jackoff writes to it until it terminates. If scheduled
    recording is desired, Jackoff can be controlled by the
    [Permanence][permanence] recording scheduler daemon.
  - Automatic shutdown. You can instruct Jackoff to only record for a certain
    number of seconds, after which Jackoff will stop recording and exit.
  - No support for writing MP2 or MP3 files.
  - Records an arbitrary number of channels. Rotter can only record mono or
    stereo audio.

This should certainly be considered alpha-quality software, given that this is
the first time I've touched C in years.

[jack]: http://www.jackaudio.org/
[libsndfile]: http://www.mega-nerd.com/libsndfile/
[rotter]: http://www.aelius.com/njh/rotter/
[permanence]: http://github.com/enaeseth/permanence

Usage
-----

Basic usage of Jackoff is very simple. To record stereo audio from the first
two JACK ports, encoded in [FLAC][FLAC] format, run:

    jackoff --auto-connect --format flac recording.flac
    # or, with less typing:
    jackoff -a -f flac recording.flac

If no format is provided, Jackoff defaults to recording in 16-bit PCM
[AIFF][AIFF]. Jackoff makes no attempt to guess at the desired format based on
the extension of the filename it is given.

To record from a given set of JACK output ports, pass the `-p` (`--ports`) option:

    jackoff -p system:capture_0,system:capture_1 recording.aiff

Jackoff will automatically create a recording with as many channels as output ports it was given to record from.

For more usage information, including a list of supported output formats, run
`jackoff --help`.

[aiff]: http://en.wikipedia.org/wiki/Audio_Interchange_File_Format
[flac]: http://en.wikipedia.org/wiki/Free_Lossless_Audio_Codec

License
-------

Jackoff is made available under the terms of the GNU General Public License.

Copyright © 2009-2010 [Eric Naeseth][copyright_holder]

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

[copyright_holder]: http://github.com/enaeseth/
