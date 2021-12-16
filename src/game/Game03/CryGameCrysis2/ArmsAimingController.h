#ifndef ARMS_AIMING_CONTROLLER_H_DEFINED
#define ARMS_AIMING_CONTROLLER_H_DEFINED


struct SPlayerParams;
struct SMovementState;
struct ICharacterInstance;

class CArmsAimingController
{
public:
	CArmsAimingController();

public:
	void Update(const bool aimEnabled, const SPlayerParams& params, const SMovementState& curMovementState, ICharacterInstance& characterInstance);

private:
};

#endif //ARMS_AIMING_CONTROLLER_H_DEFINED