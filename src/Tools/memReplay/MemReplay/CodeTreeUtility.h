#pragma once

class GenericTree;

SharedPtr<GenericTree> ReverseCode(const GenericTree& other, const std::vector<TAddress>& addresses);
SharedPtr<GenericTree> GatherCodeTopDown(const GenericTree& other, const std::vector<TAddress>& addresses);
SharedPtr<GenericTree> GatherCodeBottomUp(const GenericTree& other, const std::vector<TAddress>& addresses);
SharedPtr<GenericTree> ExcludeCode(const GenericTree& other, const std::vector<TAddress>& addresses);
