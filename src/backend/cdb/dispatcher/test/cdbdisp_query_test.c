#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include "cmockery.h"

#include "../cdbdisp_query.c"

/*
 * Mocked object initializations required for dispatchPlan.
 */
void
_init_cdbdisp_buildPlanQueryParms(QueryDesc *queryDesc)
{
	queryDesc->estate = (struct EState *)palloc0(sizeof(struct EState));
	queryDesc->estate->es_sliceTable =
		(struct SliceTable *) palloc0(sizeof(struct SliceTable));
	queryDesc->operation = CMD_NOTHING;
	queryDesc->plannedstmt = (PlannedStmt *)palloc0(sizeof(PlannedStmt));
	queryDesc->plannedstmt->planTree = (struct Plan *)palloc0(sizeof(struct Plan));

	expect_any(RootSliceIndex, estate);
	will_return(RootSliceIndex,0);
}

/*
 * Test that cdbdisp_dispatchPlan handles a plan size overflow well
 */
void
test__cdbdisp_buildPlanQueryParms__Overflow_plan_size_in_kb(void **state)
{
	bool success = false;

	struct QueryDesc *queryDesc = (struct QueryDesc *)
		palloc0(sizeof(QueryDesc));

	_init_cdbdisp_buildPlanQueryParms(queryDesc);

	/*
	 * Set max plan to a value that will require handling INT32
	 * overflow of the current plan size
	 */
	gp_max_plan_size = 1024;

	will_assign_value(serializeNode, uncompressed_size_out, INT_MAX-1);
	expect_any(serializeNode, node);
	expect_any(serializeNode, size);
	expect_any(serializeNode, uncompressed_size_out);

	will_return(serializeNode, NULL);

	PG_TRY();
	{
		cdbdisp_buildPlanQueryParms(queryDesc, false);
	}
	PG_CATCH();
	{
		/*
		 * Verify that we get the correct error (limit exceeded)
		 * CopyErrorData() requires us to get out of ErrorContext
		 */
		CurrentMemoryContext = TopMemoryContext;

		ErrorData *edata = CopyErrorData();

		StringInfo message = makeStringInfo();
		appendStringInfo(message,
						 "Query plan size limit exceeded, current size: " UINT64_FORMAT "KB, max allowed size: 1024KB",
						 ((INT_MAX-1)/(uint64)1024));

		if (edata->elevel == ERROR &&
			strncmp(edata->message, message->data, message->len) == 0)
		{
			success = true;
		}

	}
	PG_END_TRY();

	assert_true(success);
}

int
main(int argc, char* argv[])
{
	cmockery_parse_arguments(argc, argv);

	const UnitTest tests[] =
	{
		unit_test(test__cdbdisp_buildPlanQueryParms__Overflow_plan_size_in_kb)
	};

	/* There are assertions in dispatch code for this */
	Gp_role = GP_ROLE_DISPATCH;
	MemoryContextInit();

	return run_tests(tests);
}
