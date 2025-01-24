#pragma once

///@brief Definitions and all that just to make everything type safe
class Bitwise {
public:
	template<class Base_T, class Flag_T>
	static constexpr inline bool HasFlag(Base_T base, Flag_T flag) {
		return (base & flag) == flag; //And just pray this compiles every time lol
	}

	template<class Base_T, class Flag_T>
	static constexpr inline bool HasCompositeFlag(Base_T base, Flag_T compositeFlag) {
		return (base & compositeFlag) != 0; //And just pray this compiles every time lol
	}
};
