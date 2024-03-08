::
:: The following license and copyright notice applies to all content in
:: this repository where some other license does not take precedence. In
:: particular, notices in sub-project directories and individual source
:: files take precedence over this file.
::
:: See COPYING for more information.
::
:: eXtensible, Modular Hypervisor Framework 64 (XMHF64)
:: Copyright (c) 2023 Eric Li
:: All Rights Reserved.
::
:: Redistribution and use in source and binary forms, with or without
:: modification, are permitted provided that the following conditions
:: are met:
::
:: Redistributions of source code must retain the above copyright
:: notice, this list of conditions and the following disclaimer.
::
:: Redistributions in binary form must reproduce the above copyright
:: notice, this list of conditions and the following disclaimer in
:: the documentation and/or other materials provided with the
:: distribution.
::
:: Neither the name of the copyright holder nor the names of
:: its contributors may be used to endorse or promote products derived
:: from this software without specific prior written permission.
::
:: THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
:: CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
:: INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
:: MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
:: DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
:: BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
:: EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
:: TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
:: DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
:: ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
:: TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
:: THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
:: SUCH DAMAGE.
::

echo Hello world

:: Identify OS type

F:\test32.exe 1000700064
F:\test32.exe 1000700064
F:\test32.exe 1000700064

:: Test i386 pal_demo

F:\test32.exe 1100000032
F:\test32.exe 1100000032
F:\test32.exe 1100000032

F:\test_args32.exe 7 7 7

IF %ERRORLEVEL% EQU 0 GOTO success32
F:\test32.exe 1200000032
F:\test32.exe 1200000032
F:\test32.exe 1200000032
F:\test32.exe 1444444444
pause
exit 1

:success32
F:\test32.exe 1300000032
F:\test32.exe 1300000032
F:\test32.exe 1300000032

:: Test amd64 pal_demo

F:\test64.exe 1100000064
F:\test64.exe 1100000064
F:\test64.exe 1100000064

F:\test_args64.exe 7 7 7

IF %ERRORLEVEL% EQU 0 GOTO success64
F:\test64.exe 1200000064
F:\test64.exe 1200000064
F:\test64.exe 1200000064
F:\test64.exe 1444444444
pause
exit 1

:success64
F:\test64.exe 1300000064
F:\test64.exe 1300000064
F:\test64.exe 1300000064

:: End test

F:\test32.exe 1555555555
F:\test32.exe 1555555555
F:\test32.exe 1555555555

echo Bye world
pause
exit 0

