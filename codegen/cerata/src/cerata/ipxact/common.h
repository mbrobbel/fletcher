// // Copyright 2018 Delft University of Technology
// //
// // Licensed under the Apache License, Version 2.0 (the "License");
// // you may not use this file except in compliance with the License.
// // You may obtain a copy of the License at
// //
// //     http://www.apache.org/licenses/LICENSE-2.0
// //
// // Unless required by applicable law or agreed to in writing, software
// // distributed under the License is distributed on an "AS IS" BASIS,
// // WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// // See the License for the specific language governing permissions and
// // limitations under the License.


// namespace cerata::ipxact {

// // Element name for display purposes. Typically a few words providing a more 
// // detailed and/or user-friendly name than the ipxact:name.
// typedef std::string displayName;

// // Full description string, typically for documentation
// typedef std::string description;

// /// @brief A name value pair.  The name is specified by the name element.  The
// /// value is in the text content of the value element.  This value element 
// /// supports all configurability attributes.
// struct Parameter {

// 	Parameter(std::shared_ptr<Parameter> &parameter);

// 	tinyxml2::XMLElement ToElement();
// }

// /// @brief A collection of parameters and associated value assertions.
// struct Parameters {
// 	std::vector<Parameter> inner;
// }


// // Expression that determines whether the enclosing element should be treated 
// // as present (expression evaluates to "true") or disregarded (expression 
// // evalutes to "false")
// typedef bool isPresent;

// // A group of elements for name(xs:string), displayName and description
// struct nameGroupString {
// 	// Unique name
// 	std::string name;

// 	std::optional<displayName> displayName;
// 	std::optional<description> description;
// }

// // Name and value type for use in resolvable elements
// struct parameterBaseType: nameGroupString {
// 	complexBaseExpression value;

// 	// Attributes

// 	// ID attribute for uniquely identifying a parameter within its document. 
// 	// Attribute is used to refer to this from a configurable element.
// 	std::optional<std::string> parameterId;
// }


// struct parameterType: parameterBaseType {
	
// }

// // Name value pair with data type information.
// struct moduleParameterType : parameterType {
// 	std::optional<isPresent> isPresent;

// 	// Attributes
	
// 	// The data type of the argument as pertains to the language. Example: 
// 	// "int", "double", "char *".
// 	std::optional<std::string> dataType;

// 	// Indicates the type of the module parameter. Legal values are defined 
// 	// in the attribute enumeration list. Default value is 'nontyped'.
// 	std::optional<std::string> usageType;
// }

// }
