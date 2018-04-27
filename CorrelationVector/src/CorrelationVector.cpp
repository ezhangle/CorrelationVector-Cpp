//---------------------------------------------------------------------
// <copyright file="CorrelationVector.cpp" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//---------------------------------------------------------------------
#include "CorrelationVector.h"
#include "Utilities.h"
#include "InternalErrors.h"
#include <ctime>
#include <chrono>
#include <vector>
#include <string>

#define isEmptyOrWhiteSpace(str)	str.empty() || str.find_first_not_of(' ') == str.npos

namespace Microsoft
{
	bool CorrelationVector::ValidateCorrelationVectorDuringCreation;
	const std::string CorrelationVector::HEADER_NAME = "MS-CV";

	CorrelationVector::CorrelationVector()
		: CorrelationVector(CorrelationVectorVersion::V1)
	{
	}

	CorrelationVector::CorrelationVector(Microsoft::Guid guid)
		: CorrelationVector(getBaseFromGuid(&guid), 0, CorrelationVectorVersion::V2)
	{
	}

	CorrelationVector::CorrelationVector(CorrelationVectorVersion version)
		: CorrelationVector(CorrelationVector::getUniqueValue(version), 0, version)
	{
	}

	CorrelationVector::CorrelationVector(const CorrelationVector &cV)
	{
		this->baseVector = cV.baseVector;
		this->extension = cV.extension.load();
		this->correlationVectorVersion = cV.correlationVectorVersion;
	}

	CorrelationVector::CorrelationVector(std::string baseVector, int extension, CorrelationVectorVersion version)
	{
		this->baseVector = baseVector;
		this->extension = extension;
		this->correlationVectorVersion = version;
	}

	bool CorrelationVector::getValidateCorrelationVectorDuringCreation() 
	{
		return CorrelationVector::ValidateCorrelationVectorDuringCreation;
	}

	void CorrelationVector::setValidateCorrelationVectorDuringCreation(bool value)
	{
		CorrelationVector::ValidateCorrelationVectorDuringCreation = value;
	}

	std::string CorrelationVector::getBaseFromGuid(Microsoft::Guid* guid)
	{
		return guid->toBase64String().substr(0, CorrelationVector::BASE_LENGTH_V2);
	}

	std::string CorrelationVector::getUniqueValue(CorrelationVectorVersion version)
	{
		if (CorrelationVectorVersion::V1 == version)
		{
			Microsoft::Guid guid = Microsoft::Guid::newGuid();
			return guid.toBase64String(12);
		}
		else if (CorrelationVectorVersion::V2 == version)
		{
			Microsoft::Guid guid = Microsoft::Guid::newGuid();
			return guid.toBase64String();
		}
		else
		{
			throw std::invalid_argument("Unsupported correlation vector version: " + (int)version);
		}
	}

	CorrelationVectorVersion CorrelationVector::inferVersion(std::string correlationVector, bool reportErrors)
	{
		size_t index = correlationVector.empty() ? -1 : correlationVector.find_first_of('.');

		if (BASE_LENGTH == index)
		{
			return CorrelationVectorVersion::V1;
		}
		else if (BASE_LENGTH_V2 == index)
		{
			return CorrelationVectorVersion::V2;
		}
		else
		{
			if (reportErrors)
			{
				Microsoft::InternalErrors::reportError("Invalid correlation vector " + correlationVector);
			}

			// Fallback to V1 implementation for invalid cVs
			return CorrelationVectorVersion::V1;
		}
	}

	void CorrelationVector::validate(std::string correlationVector, CorrelationVectorVersion version)
	{
		try
		{
			size_t maxVectorLength;
			size_t baseLength;

			if (CorrelationVectorVersion::V1 == version)
			{
				maxVectorLength = CorrelationVector::MAX_VECTOR_LENGTH;
				baseLength = CorrelationVector::BASE_LENGTH;
			}
			else if (CorrelationVectorVersion::V2 == version)
			{
				maxVectorLength = CorrelationVector::MAX_VECTOR_LENGTH_V2;
				baseLength = CorrelationVector::BASE_LENGTH_V2;
			}
			else
			{
				throw std::invalid_argument("Unsupported correlation vector version: " + (int)version);
			}

			if (isEmptyOrWhiteSpace(correlationVector) || correlationVector.length() > maxVectorLength)
			{
				throw std::invalid_argument("The " + correlationVector + " correlation vector can not be null or bigger than " + std::to_string(maxVectorLength) + " characters");
			}

			std::vector<std::string> parts = split_str(correlationVector, '.');

			size_t length = parts.size();
			if (length < (size_t)2 || parts[0].length() != baseLength)
			{
				throw std::invalid_argument("Invalid correlation vector " + correlationVector + ". Invalid base value " + parts[0]);
			}

			for (size_t i = 1; i < length; ++i)
			{
				try
				{
					std::size_t lastChar;
					int result = std::stoi(parts[i], &lastChar, 10);
					if (lastChar != parts[i].length() || result < 0)
					{
						throw std::invalid_argument("Invalid correlation vector " + correlationVector + ". Invalid extension value " + parts[i]);
					}
				}
				catch (std::invalid_argument&)
				{
					throw std::invalid_argument("Invalid correlation vector " + correlationVector + ". Invalid extension value " + parts[i]);
				}
				catch (std::out_of_range&)
				{
					throw std::invalid_argument("Invalid correlation vector " + correlationVector + ". Invalid extension value " + parts[i]);
				}
			}
		}
		catch (std::invalid_argument& exception)
		{
			Microsoft::InternalErrors::reportError(exception.what());
		}
	}

	CorrelationVector CorrelationVector::extend(std::string correlationVector)
	{
		CorrelationVectorVersion version = CorrelationVector::inferVersion(
			correlationVector, CorrelationVector::ValidateCorrelationVectorDuringCreation);

		if (CorrelationVector::ValidateCorrelationVectorDuringCreation)
		{
			CorrelationVector::validate(correlationVector, version);
		}

		return CorrelationVector(correlationVector, 0, version);
	}

	CorrelationVector CorrelationVector::spin(std::string correlationVector)
	{
		return CorrelationVector::spin(correlationVector, SpinParameters::getDefaultSpinParameters());
	}

	CorrelationVector CorrelationVector::spin(std::string correlationVector, SpinParameters parameters)
	{
		CorrelationVectorVersion version = CorrelationVector::inferVersion(
			correlationVector, CorrelationVector::ValidateCorrelationVectorDuringCreation);

		if (CorrelationVector::ValidateCorrelationVectorDuringCreation)
		{
			CorrelationVector::validate(correlationVector, version);
		}

		int entropyBytes = parameters.getEntropyBytes();
		unsigned char* entropy = new unsigned char[entropyBytes]();
		
		std::srand((unsigned int)std::time(nullptr));

		for (int i = 0; i < entropyBytes; ++i)
		{
			entropy[i] = (unsigned char)rand();
		}

		long long ticks = std::chrono::system_clock::now().time_since_epoch().count();
		long long value = ticks >> parameters.getTicksBitsToDrop();
		for (int i = 0; i < entropyBytes; ++i)
		{
			value = (value << 8) | ((uint64_t)entropy[i]);
		}
		int totalBits = parameters.getTotalBits();
		value &= (totalBits == 64 ? 0 : (long long)1 << totalBits) - 1;

		std::string s = std::to_string((unsigned int)value);
		if (totalBits > 32)
		{
			s = std::to_string((unsigned int)(value >> 32)) + '.' + s;
		}

		return CorrelationVector(correlationVector + '.' + s, 0, version);
	}

	CorrelationVector CorrelationVector::parse(std::string correlationVector)
	{
		if (!correlationVector.empty())
		{
			int p = correlationVector.find_last_of('.');
			if (p > 0)
			{
				std::string lastStage = correlationVector.substr(p + 1);
				try
				{
					std::size_t lastChar;
					int extension = std::stoi(lastStage, &lastChar, 10);
					if (lastChar == lastStage.length() && extension >= 0)
					{
						CorrelationVectorVersion version = CorrelationVector::inferVersion(correlationVector, false);
						return CorrelationVector(correlationVector.substr(0, p), extension, version);
					}
				}
				catch (std::invalid_argument)
				{
				}
				catch (std::out_of_range&)
				{
				}
			}
		}
		return CorrelationVector();
	}

	std::string CorrelationVector::getValue()
	{
		return this->baseVector + '.' + std::to_string(this->extension);
	}

	std::string CorrelationVector::increment()
	{
		int snapshot = 0;
		int next = 0;

		do
		{
			snapshot = this->extension.load();
			if (snapshot == INT_MAX)
			{
				return this->getValue();
			}
			next = snapshot + 1;
			int size = baseVector.length() + 1 + (int)std::log10(next) + 1;
			if ((this->getVersion() == CorrelationVectorVersion::V1 &&
				size > CorrelationVector::MAX_VECTOR_LENGTH) ||
				(this->getVersion() == CorrelationVectorVersion::V2 &&
					size > CorrelationVector::MAX_VECTOR_LENGTH_V2))
			{
				return this->getValue();
			}
		} while (!this->extension.compare_exchange_weak(snapshot, next));

		return this->baseVector + '.' + std::to_string(next);
	}

	CorrelationVectorVersion CorrelationVector::getVersion()
	{
		return this->correlationVectorVersion;
	}

	std::string CorrelationVector::toString()
	{
		return this->getValue();
	}

	bool CorrelationVector::equals(CorrelationVector vector)
	{
		return this->baseVector == vector.baseVector && this->extension == vector.extension;
	}
}

