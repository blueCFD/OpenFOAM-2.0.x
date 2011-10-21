@echo off
rem ------------------------------------------------------------------------------
rem  License
rem     This file is part of blueCAPE's unofficial mingw patches for OpenFOAM.
rem     For more information about these patches, visit:
rem         http://www.bluecape.com.pt/blueCFD
rem
rem     OpenFOAM's official site: http://www.openfoam.com
rem
rem     OpenFOAM is free software: you can redistribute it and/or modify it
rem     under the terms of the GNU General Public License as published by
rem     the Free Software Foundation, either version 3 of the License, or
rem     (at your option) any later version.
rem
rem     OpenFOAM is distributed in the hope that it will be useful, but WITHOUT
rem     ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
rem     FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
rem     for more details.
rem
rem     You should have received a copy of the GNU General Public License
rem     along with OpenFOAM.  If not, see <http://www.gnu.org/licenses/>.
rem 
rem  Script
rem      gompi.bat
rem 
rem  Description
rem      This batch file is analogous to foamJob, but for Windows Command Line.
rem 
rem ------------------------------------------------------------------------------
@echo on
@set /A x_numprocs=0
@for /D %%a in (processor*) do @set /A x_numprocs=x_numprocs+1

@GOTO %WM_MPLIB%

:OPENMPI
@for /f "delims=" %%a in ('where %1') do @set APPLICATIONTORUN=%%a
mpirun -n %x_numprocs% -x HOME -x PATH -x USERNAME -x WM_PROJECT_DIR -x WM_PROJECT_INST_DIR -x WM_OPTIONS -x FOAM_LIBBIN -x FOAM_APPBIN -x FOAM_USER_APPBIN -x MPI_BUFFER_SIZE %APPLICATIONTORUN% -parallel %2 %3 %4 %5 %6 %7 %8 %9
@GOTO end


:MSMPI
:MPICH
mpiexec -n %x_numprocs% -genvlist HOME,PATH,USERNAME,WM_PROJECT_DIR,WM_PROJECT_INST_DIR,WM_OPTIONS,FOAM_LIBBIN,FOAM_APPBIN,FOAM_USER_APPBIN,MPI_BUFFER_SIZE %1 -parallel %2 %3 %4 %5 %6 %7 %8 %9
@GOTO end


:end
@set x_numprocs=
