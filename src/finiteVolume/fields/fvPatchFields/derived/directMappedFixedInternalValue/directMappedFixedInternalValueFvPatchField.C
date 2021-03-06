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

\*---------------------------------------------------------------------------*/

#include "directMappedFixedInternalValueFvPatchField.H"
#include "UIndirectList.H"

// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

template<class Type>
Foam::directMappedFixedInternalValueFvPatchField<Type>::
directMappedFixedInternalValueFvPatchField
(
    const fvPatch& p,
    const DimensionedField<Type, volMesh>& iF
)
:
    directMappedFixedValueFvPatchField<Type>(p, iF)
{}


template<class Type>
Foam::directMappedFixedInternalValueFvPatchField<Type>::
directMappedFixedInternalValueFvPatchField
(
    const directMappedFixedInternalValueFvPatchField<Type>& ptf,
    const fvPatch& p,
    const DimensionedField<Type, volMesh>& iF,
    const fvPatchFieldMapper& mapper
)
:
    directMappedFixedValueFvPatchField<Type>(ptf, p, iF, mapper)
{}


template<class Type>
Foam::directMappedFixedInternalValueFvPatchField<Type>::
directMappedFixedInternalValueFvPatchField
(
    const fvPatch& p,
    const DimensionedField<Type, volMesh>& iF,
    const dictionary& dict
)
:
    directMappedFixedValueFvPatchField<Type>(p, iF, dict)
{}


template<class Type>
Foam::directMappedFixedInternalValueFvPatchField<Type>::
directMappedFixedInternalValueFvPatchField
(
    const directMappedFixedInternalValueFvPatchField<Type>& ptf
)
:
    directMappedFixedValueFvPatchField<Type>(ptf)
{}


template<class Type>
Foam::directMappedFixedInternalValueFvPatchField<Type>::
directMappedFixedInternalValueFvPatchField
(
    const directMappedFixedInternalValueFvPatchField<Type>& ptf,
    const DimensionedField<Type, volMesh>& iF
)
:
    directMappedFixedValueFvPatchField<Type>(ptf, iF)
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

template<class Type>
void Foam::directMappedFixedInternalValueFvPatchField<Type>::updateCoeffs()
{
    typedef GeometricField<Type, fvPatchField, volMesh> FieldType;

    if (this->updated())
    {
        return;
    }

    // Since we're inside initEvaluate/evaluate there might be processor
    // comms underway. Change the tag we use.
    int oldTag = UPstream::msgType();
    UPstream::msgType() = oldTag+1;

    // Retrieve the neighbour values and assign to this patch boundary field
    directMappedFixedValueFvPatchField<Type>::updateCoeffs();

    // Get the coupling information from the directMappedPatchBase
    const directMappedPatchBase& mpp =
        refCast<const directMappedPatchBase>(this->patch().patch());
    const mapDistribute& distMap = mpp.map();
    const fvMesh& nbrMesh = refCast<const fvMesh>(mpp.sampleMesh());

    Field<Type> nbrIntFld;

    switch (mpp.mode())
    {
        case directMappedPatchBase::NEARESTCELL:
        {
            FatalErrorIn
            (
                "void directMappedFixedValueFvPatchField<Type>::"
                "updateCoeffs()"
            )<< "Cannot apply "
             << directMappedPatchBase::sampleModeNames_
                [
                    directMappedPatchBase::NEARESTCELL
                ]
             << " mapping mode for patch " << mpp.samplePatch()
             << exit(FatalError);

            break;
        }
        case directMappedPatchBase::NEARESTPATCHFACE:
        {
            const label samplePatchI = mpp.samplePolyPatch().index();
            const fvPatchField<Type>& nbrPatchField =
                this->sampleField().boundaryField()[samplePatchI];
            nbrIntFld = nbrPatchField.patchInternalField();
            distMap.distribute(nbrIntFld);

            break;
        }
        case directMappedPatchBase::NEARESTFACE:
        {
            Field<Type> allValues(nbrMesh.nFaces(), pTraits<Type>::zero);

            const FieldType& nbrField = this->sampleField();

            forAll(nbrField.boundaryField(), patchI)
            {
                const fvPatchField<Type>& pf = nbrField.boundaryField()[patchI];
                const Field<Type> pif(pf.patchInternalField());

                label faceStart = pf.patch().start();

                forAll(pf, faceI)
                {
                    allValues[faceStart++] = pif[faceI];
                }
            }

            distMap.distribute(allValues);
            nbrIntFld.transfer(allValues);

            break;
        }
        default:
        {
            FatalErrorIn
            (
                "directMappedFixedValueFvPatchField<Type>::updateCoeffs()"
            )<< "Unknown sampling mode: " << mpp.mode()
             << nl << abort(FatalError);
        }
    }


    // Restore tag
    UPstream::msgType() = oldTag;

    // Assign (this) patch internal field to its neighbour values
    Field<Type>& intFld = const_cast<Field<Type>&>(this->internalField());
    UIndirectList<Type>(intFld, this->patch().faceCells()) = nbrIntFld;
}


template<class Type>
void Foam::directMappedFixedInternalValueFvPatchField<Type>::write
(
    Ostream& os
) const
{
    directMappedFixedValueFvPatchField<Type>::write(os);
}


// ************************************************************************* //
