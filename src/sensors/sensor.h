/*
	SlimeVR Code is placed under the MIT license
	Copyright (c) 2021 Eiren Rain & SlimeVR contributors

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in
	all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
	THE SOFTWARE.
*/

#ifndef SLIMEVR_SENSOR_H_
#define SLIMEVR_SENSOR_H_

#include <Arduino.h>
#include <quat.h>
#include <vector3.h>

#include <memory>

#include "PinInterface.h"
#include "SensorToggles.h"
#include "configuration/Configuration.h"
#include "globals.h"
#include "logging/Logger.h"
#include "sensorinterface/RegisterInterface.h"
#include "sensorinterface/SensorInterface.h"
#include "sensorinterface/i2cimpl.h"
#include "status/TPSCounter.h"
#include "utils.h"

#define DATA_TYPE_NORMAL 1
#define DATA_TYPE_CORRECTION 2

enum class SensorStatus : uint8_t {
	SENSOR_OFFLINE = 0,
	SENSOR_OK = 1,
	SENSOR_ERROR = 2
};

class Sensor {
public:
	Sensor(
		const char* sensorName,
		SensorTypeID type,
		uint8_t id,
		SlimeVR::Sensors::RegisterInterface& registerInterface,
		float rotation,
		SlimeVR::SensorInterface* sensorInterface = nullptr
	)
		: m_hwInterface(sensorInterface)
		, m_RegisterInterface(registerInterface)
		, sensorId(id)
		, sensorType(type)
		, sensorOffset({Quat(Vector3(0, 0, 1), rotation)})
		, m_Logger(SlimeVR::Logging::Logger(sensorName)) {
		char buf[4];
		sprintf(buf, "%u", id);
		m_Logger.setTag(buf);
		addr = registerInterface.getAddress();
	}

	virtual ~Sensor(){};
	virtual void motionSetup(){};
	virtual void postSetup(){};
	virtual void motionLoop(){};
	virtual void sendData();
	virtual void setAcceleration(Vector3 a);
	virtual void setFusedRotation(Quat r);
	virtual void startCalibration(int calibrationType){};
	virtual SensorStatus getSensorState();
	virtual void printTemperatureCalibrationState();
	virtual void printDebugTemperatureCalibrationState();
	virtual void resetTemperatureCalibrationState();
	virtual void saveTemperatureCalibration();
	// TODO: currently only for softfusionsensor, bmi160 and others should get
	// an overload too
	virtual const char* getAttachedMagnetometer() const;
	// TODO: realistically each sensor should print its own state instead of
	// having 15 getters for things only the serial commands use
	bool isWorking() { return working; };
	bool getHadData() const { return hadData; };
	bool isValid() { return m_hwInterface != nullptr; };
	uint8_t getSensorId() { return sensorId; };
	SensorTypeID getSensorType() { return sensorType; };
	const Vector3& getAcceleration() { return acceleration; };
	const Quat& getFusedRotation() { return fusedRotation; };
	bool hasNewDataToSend() { return newFusedRotation || newAcceleration; };
	inline bool hasCompletedRestCalibration() { return restCalibrationComplete; }
	void setFlag(SensorToggles toggle, bool state);
	[[nodiscard]] virtual bool isFlagSupported(SensorToggles toggle) const {
		return false;
	}
	SlimeVR::Configuration::SensorConfigBits getSensorConfigData();

	virtual SensorDataType getDataType() {
		return SensorDataType::SENSOR_DATATYPE_ROTATION;
	};

	SensorPosition getSensorPosition() { return m_SensorPosition; };

	void setSensorInfo(SensorPosition sensorPosition) {
		m_SensorPosition = sensorPosition;
	};

	TPSCounter m_tpsCounter;
	TPSCounter m_dataCounter;
	SlimeVR::SensorInterface* m_hwInterface = nullptr;

protected:
	SlimeVR::Sensors::RegisterInterface& m_RegisterInterface;
	uint8_t addr;
	uint8_t sensorId = 0;
	SensorTypeID sensorType = SensorTypeID::Unknown;
	bool working = false;
	bool hadData = false;
	uint8_t calibrationAccuracy = 0;
	/**
	 * Apply sensor offset to align it with tracker's axises
	 * (Y to top of the tracker, Z to front, X to left)
	 */
	Quat sensorOffset;

	bool newFusedRotation = false;
	Quat fusedRotation{};
	Quat lastFusedRotationSent{};

	bool newAcceleration = false;
	Vector3 acceleration{};

	SensorPosition m_SensorPosition = SensorPosition::POSITION_NO;

	SensorToggleState toggles;

	void markRestCalibrationComplete(bool completed = true);

	mutable SlimeVR::Logging::Logger m_Logger;

private:
	void printTemperatureCalibrationUnsupported();

	bool restCalibrationComplete = false;
};

const char* getIMUNameByType(SensorTypeID imuType);

#endif  // SLIMEVR_SENSOR_H_
