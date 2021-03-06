/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | Copyright (C) 2011 OpenFOAM Foundation
     \\/     M anipulation  |
-------------------------------------------------------------------------------
License
    This file is part of OpenFOAM.

    OpenFOAM is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    OpenFOAM is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with OpenFOAM.  If not, see <http://www.gnu.org/licenses/>.

Class
    Foam::explicitSource

Description
    Explicit source.

    Sources described by:

    explicitSourceCoeffs
    {
        points            // list of points when selectionMode = points
        (
            (-0.088 0.007 -0.02)
            (-0.028 0.007 -0.02)
        );
        volumeMode          specific;  //absolute
        fieldData            // field data - usage for multiple fields
        {
            k   30.7;
            epsilon  1.5;
        }
    }

SourceFiles
    explicitSource.C

\*---------------------------------------------------------------------------*/

#ifndef explicitSource_H
#define explicitSource_H

#include "cellSet.H"
#include "volFieldsFwd.H"
#include "DimensionedField.H"
#include "basicSource.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

/*---------------------------------------------------------------------------*\
                       Class explicitSource Declaration
\*---------------------------------------------------------------------------*/

class explicitSource
:
    public basicSource
{
    // Private data

        //- List of field types
        HashTable<scalar> scalarFields_;
        HashTable<vector> vectorFields_;

        //- Add source to matrix
        template<class Type>
        void addSource(fvMatrix<Type>&, const Type&) const;

        //- Add field names and values to field table for types.
        template<class Type>
        void addField
        (
            HashTable<Type>& fields,
            const wordList& fieldTypes,
            const wordList& fieldNames,
            const dictionary& dict
        );


public:

    // Public data

        //- Enumeration for volume types
        enum volumeModeType
        {
            vmAbsolute,
            vmSpecific
        };

        //- Word list of volume mode type names
        static const NamedEnum<volumeModeType, 2> volumeModeTypeNames_;


protected:

    // Protected data

         //- Sub dictionary for time activated explicit sources
        const dictionary& dict_;

        //- Volume mode
        volumeModeType volumeMode_;


    // Protected functions

        //- Set the local field data
        void setFieldData(const dictionary& dict);


public:

    //- Runtime type information
    TypeName("explicitSource");


    // Constructors

        //- Construct from components
        explicitSource
        (
            const word& name,
            const word& modelType,
            const dictionary& dict,
            const fvMesh& mesh
        );

        //- Return clone
        autoPtr<explicitSource> clone() const
        {
            notImplemented
            (
                "autoPtr<explicitSource> clone() const"
            );
            return autoPtr<explicitSource>(NULL);
        }



    // Member Functions

        // Access

            //- Return const access to the volume mode
            inline const volumeModeType& volumeMode() const;


        // Edit

            //- Return access to the volume mode
            inline volumeModeType& volumeMode();

            //- Return points
            inline  const List<point>& points() const;


        // Evaluation

            //-Source term to fvMatrix<vector>
            virtual void addSu(fvMatrix<vector>& UEqn);

            //-Source term to fvMatrix<scalar>
            virtual void addSu(fvMatrix<scalar>& UEqn);


        // I-O

            //- Write the source properties
            virtual void writeData(Ostream&) const;

            //- Read fieldData in sub-dictionary
            virtual bool read(const dictionary& dict);

            //- Ostream operator
            friend Ostream& operator<<
            (
                Ostream& os,
                const explicitSource& source
            );
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#include "explicitSourceIO.C"
#include "explicitSourceI.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#ifdef NoRepository
#   include "explicitSourceTemplates.C"
#endif

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
