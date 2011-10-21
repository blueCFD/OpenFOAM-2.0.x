@echo off
rem ------------------------------------------------------------------------------
rem  =========                 |
rem  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
rem   \\    /   O peration     |
rem    \\  /    A nd           | Copyright (C) 1991-2010 OpenCFD Ltd.
rem     \\/     M anipulation  |
rem ------------------------------------------------------------------------------
rem  License
rem      This file is part of blueCAPE's unofficial mingw patches for OpenFOAM.
rem 
rem      OpenFOAM is free software: you can redistribute it and/or modify it
rem      under the terms of the GNU General Public License as published by
rem      the Free Software Foundation, either version 3 of the License, or
rem      (at your option) any later version.
rem 
rem      OpenFOAM is distributed in the hope that it will be useful, but WITHOUT
rem      ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
rem      FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
rem      for more details.
rem 
rem      You should have received a copy of the GNU General Public License
rem      along with OpenFOAM.  If not, see <http://www.gnu.org/licenses/>.
rem 
rem  Script
rem      paraFoam.bat
rem 
rem  Description
rem      This batch file is analogous to paraFoam, but for Windows Command Line.
rem 
rem ------------------------------------------------------------------------------

set caseName=case
if "%1" == "-case" goto folderg

echo.>%caseName%.foam
start paraview.exe --data="%caseName%.foam"
goto end

:folderg
if "%2" == "" goto end

set caseName=%2
echo.>%caseName%\%caseName%.foam
start paraview.exe --data="%caseName%\%caseName%.foam"

:end
