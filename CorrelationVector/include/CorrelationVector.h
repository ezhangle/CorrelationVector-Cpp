﻿//---------------------------------------------------------------------
// <copyright file="CorrelationVector.h" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//---------------------------------------------------------------------
#pragma once
#include <string>
#include <atomic>
#include "Guid.h"
#include "SpinParameters.h"
#include "export.h"

namespace Microsoft 
{
    enum class EXPORTABLE CorrelationVectorVersion {
		V1, 
		V2
    };

    class CorrelationVector
    {
    private:
        CorrelationVectorVersion correlationVectorVersion;
        static const size_t MAX_VECTOR_LENGTH = 63;
        static const size_t MAX_VECTOR_LENGTH_V2 = 127;
        static const size_t BASE_LENGTH = 16;
        static const size_t BASE_LENGTH_V2 = 22;

		static bool ValidateCorrelationVectorDuringCreation;

        std::string baseVector;
        std::atomic<int> extension = 0;
		bool isCvImmutable;

        CorrelationVector(std::string baseVector, int extension, CorrelationVectorVersion version, bool isImmutable);

        static std::string getBaseFromGuid(Microsoft::Guid* guid);
        static std::string getUniqueValue(CorrelationVectorVersion version);
        static CorrelationVectorVersion inferVersion(std::string correlationVector, bool reportErrors);
        static void validate(std::string correlationVector, CorrelationVectorVersion version);
		static int intLength(int i);
		static bool isImmutable(std::string correlationVector);
		static bool isOversized(std::string baseVector, int extension, CorrelationVectorVersion version);

    public:

		/**
			This is the header that should be used between services to pass the 
			correlation vector
		*/
		EXPORTABLE static const std::string HEADER_NAME;

		/**
			This is the delimiter to indicate that a CV is terminated.
		*/
		EXPORTABLE static const char TERMINATOR;

		/**
			Initializes a new instance of the Correlation Vector. This should only
			be called when no Correlation Vector was found in the message header.
		*/
		EXPORTABLE CorrelationVector();

		/**
			Initializes a new instance of the Correlation Vector of the V2 implementation
			using the given Guid as the vector base. This should only be called when no 
			Correlation Vector was found in the message header.
		*/
		EXPORTABLE CorrelationVector(Microsoft::Guid guid);
		
		/**
			Initializes a new instance of the Correlation Vector of the given implementation
			version. This should only be called when no Correlation Vector was found in the
			message header.
		*/
		EXPORTABLE CorrelationVector(CorrelationVectorVersion version);
		
		/**
			Initializes a new instance of the Correlation Vector using the given Correlation
			Vector.
		*/
		EXPORTABLE CorrelationVector(const CorrelationVector &cV);

		/**
			Gets whether or not to validate the Correlation Vector on creation
			@return A boolean value representing whether or not to validate
		*/
		EXPORTABLE static bool getValidateCorrelationVectorDuringCreation();
		/**
			Sets whether or not to validate the Correlation Vector on creation
			@param value A boolean value representing whether or not to validate
		*/
		EXPORTABLE static void setValidateCorrelationVectorDuringCreation(bool value);

		/**
			Creates a new Correlation Vector by extending an existing value. This should be
			done at the entry point of an operation.
			@param The Correlation Vector taken from the message header
			@return A new Correlation Vector extended from the current vector
		*/
		EXPORTABLE static CorrelationVector extend(std::string correlationVector);

		/**
			Creates a new Correlation Vector by applying the Spin operator to an existing value.
			This should be done at the entry point of an operation.
			@param correlationVector The Correlation Vector taken from the message header
			@return A new Correlation Vector extended from the current vector
		*/
		EXPORTABLE static CorrelationVector spin(std::string correlationVector);

		/**
			Creates a new Correlation Vector by applying the Spin operator to an existing value.
			This should be done at the entry point of an operation.
			@param correlationVector The Correlation Vector taken from the message header
			@param parameters The parameters to use when applying the Spin operator
			@return A new Correlation Vector extended from the current vector
		*/
		EXPORTABLE static CorrelationVector spin(std::string correlationVector, SpinParameters parameters);

		/**
			Creates a new Correlation Vector by parsing its string representation
			@param correlationVector The Correlation Vector in its string representation
			@return A new Correlation Vector parsed from its string representation
		*/
		EXPORTABLE static CorrelationVector parse(std::string correlationVector);

		/**
			Gets the value of the Correlation Vector as a string
			@return The string representation of the Correlation Vector
		*/
		EXPORTABLE std::string getValue();

		/**
			Increments the current extension by one. Do this before passing the value to an
			outbound message header.
			@return The new value as a string that you can add to the outbound message header
		*/
		EXPORTABLE std::string increment();

		/**
			Gets the version of the Correlation Vector implementation.
			@return The version of the Correlation Vector implementation
		*/
		EXPORTABLE CorrelationVectorVersion getVersion();

		/**
			Gets the value of the Correlation Vector as a string
			@return The string representation of the Correlation Vector
		*/
		EXPORTABLE std::string toString();

		/**
			Determines whether two instances of the Correlation Vector are equal
			@param vector The Correlation Vector you want to compare with the current Correlation Vector
			@result True if the specified Correlation Vector is equal to the current Correlation Vector,
			otherwise false.
		*/
		EXPORTABLE bool equals(CorrelationVector vector);
    };
}