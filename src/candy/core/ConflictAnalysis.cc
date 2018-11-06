#include "candy/utils/Options.h"

namespace Candy {

namespace ClauseLearningOptions {
	Glucose::IntOption opt_lb_size_minimzing_clause("ClauseLearning", "minSizeMinimizingClause", "The min size required to minimize clause", 30, Glucose::IntRange(3, INT16_MAX));
}

}