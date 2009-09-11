Jackoff
=======

Jackoff is a simple utility that records audio off of a [JACK][jack] source to
a file, using any of the encodings supported by [libsndfile][libsndfile].

Jackoff is heavily based on [Rotter][rotter], another program that allows you
to record audio from JACK and save it to file. The main differences are:

  - No file rotation. Rotter starts writing to a different file every hour, but
    Jackoff does not include this behavior. You pass it the name of a file you
    wish to write to and Jackoff writes to it until it terminates.
  - Automatic shutdown. You can instruct Jackoff to only record for a certain
    number of seconds, after which Jackoff will stop recording and exit.
  - No support for writing MP2 or MP3 files.
  - Records an arbitrary number of channels. Rotter can only record mono or
    stereo audio.
  - Jackoff does not yet support being given specific JACK ports to record
    from; it simply takes the first output ports it finds up to the desired
    number of audio channels.

This should certainly be considered alpha-quality software, given that this is
the first time I've touched C in years.

[jack]: http://www.jackaudio.org/
[libsndfile]: http://www.mega-nerd.com/libsndfile/
[rotter]: http://www.aelius.com/njh/rotter/

License
-------

Jackoff is made available under the terms of the GNU General Public License.

Copyright © 2009 [Eric Naeseth][copyright_holder]

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
